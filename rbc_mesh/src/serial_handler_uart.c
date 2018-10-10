/***********************************************************************************
Copyright (c) Nordic Semiconductor ASA
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  3. Neither the name of Nordic Semiconductor ASA nor the names of other
  contributors to this software may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************************************************/
#include "serial_handler.h"
#include "event_handler.h"
#include "rbc_mesh_common.h"
#include "fifo.h"

#include "nrf_soc.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "app_util_platform.h"
#include <string.h>

#define SERIAL_QUEUE_SIZE       (4)

/*****************************************************************************
* Static types
*****************************************************************************/
typedef enum
{
    SERIAL_STATE_IDLE,
    SERIAL_STATE_WAIT_FOR_QUEUE,
    SERIAL_STATE_TRANSMIT,
    SERIAL_STATE_DISABLED
} serial_state_t;
/*****************************************************************************
* Static globals
*****************************************************************************/
static fifo_t           m_rx_fifo;
static fifo_t           m_tx_fifo;
static serial_data_t    m_rx_fifo_buffer[SERIAL_QUEUE_SIZE];
static serial_data_t    m_tx_fifo_buffer[SERIAL_QUEUE_SIZE];

static serial_state_t   m_serial_state;
static serial_data_t    m_tx_buffer;
static uint32_t         m_tx_len;
static uint8_t*         mp_tx_ptr;
static bool             m_suspend;
/*****************************************************************************
* Static functions
*****************************************************************************/
static void do_transmit(void*);
#ifdef BOOTLOADER
void SWI1_IRQHandler(void)
{
    do_transmit(NULL);
}
#else
static void mesh_aci_command_check_cb(void* p_context)
{
    mesh_aci_command_check();
}
#endif


/** @brief Process packet queue, always done in the async context */
static void do_transmit(void* p_context)
{
    if (fifo_pop(&m_tx_fifo, &m_tx_buffer) == NRF_SUCCESS)
    {
        /* should be serial_evt_t->length+1, but will be decremented after the
         * push below, so we don't bother */
        m_tx_len = *m_tx_buffer.buffer;
        mp_tx_ptr = m_tx_buffer.buffer + 1;

        NRF_UART0->EVENTS_TXDRDY = 0;
        NRF_UART0->TASKS_STARTTX = 1;
        NRF_UART0->TXD = *(mp_tx_ptr++);
    }
}

/** @brief Put a do_transmit call up for asynchronous processing */
static void schedule_transmit(void)
{
    if (!m_suspend && m_serial_state != SERIAL_STATE_TRANSMIT)
    {
        m_serial_state = SERIAL_STATE_TRANSMIT;
#ifdef BOOTLOADER
        NVIC_SetPendingIRQ(SWI1_IRQn);
#else
        async_event_t evt;
        evt.type = EVENT_TYPE_GENERIC;
        evt.callback.generic.cb = do_transmit;
        evt.callback.generic.p_context = NULL;
        if (event_handler_push(&evt) != NRF_SUCCESS)
        {
            m_serial_state = SERIAL_STATE_IDLE;
        }
#endif
    }
}

static void char_rx(uint8_t c)
{
    static serial_data_t rx_buf = {0};
    static uint8_t* pp = rx_buf.buffer;
    static uint8_t in_packet = 0,
                   byte_is_escape = 0;

    if ((c & 0xf0) == 0x70)
    {
        switch(c)
        {
            case 0x71:
                in_packet = 255;
                byte_is_escape = 0;
                pp = rx_buf.buffer;
                break;
            case 0x72:
                byte_is_escape = 255;
                break;
            case 0x73:
                in_packet = 0;
                if (fifo_push(&m_rx_fifo, &rx_buf) != NRF_SUCCESS)
                {
                    /* respond inline, queue was full */
                    serial_evt_t fail_evt;
                    fail_evt.length = 3;
                    fail_evt.opcode = SERIAL_EVT_OPCODE_CMD_RSP;
                    fail_evt.params.cmd_rsp.command_opcode = ((serial_cmd_t*) rx_buf.buffer)->opcode;
                    fail_evt.params.cmd_rsp.status = ACI_STATUS_ERROR_BUSY;
                    serial_handler_event_send(&fail_evt);
                }
                else
                {
#ifdef BOOTLOADER
                    NVIC_SetPendingIRQ(SWI2_IRQn);
#else
                    async_event_t async_evt;
                    async_evt.type = EVENT_TYPE_GENERIC;
                    async_evt.callback.generic.cb = mesh_aci_command_check_cb;
                    async_evt.callback.generic.p_context = NULL;
                    event_handler_push(&async_evt);
#endif
                }

                if (fifo_is_full(&m_rx_fifo))
                {
                    m_serial_state = SERIAL_STATE_WAIT_FOR_QUEUE;
                    NRF_UART0->TASKS_STOPRX = 1;
                }
                break;
        }
    }
    else if (in_packet)
    {
        if (byte_is_escape)
            *(pp++) = c ^ 0x20;
        else
            *(pp++) = c;
        byte_is_escape = 0;
    }
}

/*****************************************************************************
* System callbacks
*****************************************************************************/
void UART0_IRQHandler(void)
{
    /* receive all pending bytes */
    while (NRF_UART0->EVENTS_RXDRDY)
    {
        NRF_UART0->EVENTS_RXDRDY = 0;
        (void) NRF_UART0->EVENTS_RXDRDY;
        char_rx(NRF_UART0->RXD);
    }

    /* transmit any pending bytes */
    if (NRF_UART0->EVENTS_TXDRDY)
    {
        NRF_UART0->EVENTS_TXDRDY = 0;
        if (m_tx_len--)
        {
            NRF_UART0->TXD = *(mp_tx_ptr++);
        }
        else
        {
            NRF_UART0->TASKS_STOPTX = 1;
            if (m_serial_state != SERIAL_STATE_WAIT_FOR_QUEUE)
            {
                m_serial_state = SERIAL_STATE_IDLE;
            }

            if (!fifo_is_empty(&m_tx_fifo))
            {
                schedule_transmit();
            }
        }
    }
}

/*****************************************************************************
* Interface functions
*****************************************************************************/

void serial_handler_init(void)
{
    /* init packet queues */
    m_tx_fifo.array_len = sizeof(m_tx_fifo_buffer) / sizeof(m_tx_fifo_buffer[0]);
    m_tx_fifo.elem_array = m_tx_fifo_buffer;
    m_tx_fifo.elem_size = sizeof(m_tx_fifo_buffer[0]);
    m_tx_fifo.memcpy_fptr = NULL;
    fifo_init(&m_tx_fifo);
    m_rx_fifo.array_len = sizeof(m_rx_fifo_buffer) / sizeof(m_rx_fifo_buffer[0]);
    m_rx_fifo.elem_array = m_rx_fifo_buffer;
    m_rx_fifo.elem_size = sizeof(m_rx_fifo_buffer[0]);
    m_rx_fifo.memcpy_fptr = NULL;
    fifo_init(&m_rx_fifo);

    m_suspend = false;

    /* setup hw */
    nrf_gpio_cfg_input(RX_PIN_NUMBER, NRF_GPIO_PIN_PULLUP);
    NRF_GPIO->OUTSET = 1 << TX_PIN_NUMBER;
    nrf_gpio_cfg_output(TX_PIN_NUMBER);

    NRF_UART0->PSELTXD       = TX_PIN_NUMBER;
    NRF_UART0->PSELRXD       = RX_PIN_NUMBER;
#if (HWFC == true)
    NRF_GPIO->OUTSET = 1 << RTS_PIN_NUMBER;
    nrf_gpio_cfg_output(RTS_PIN_NUMBER);
    nrf_gpio_cfg_input(CTS_PIN_NUMBER, NRF_GPIO_PIN_PULLUP);
    NRF_UART0->PSELCTS       = CTS_PIN_NUMBER;
    NRF_UART0->PSELRTS       = RTS_PIN_NUMBER;
    NRF_UART0->CONFIG        = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
#else
    NRF_UART0->CONFIG        = (UART_CONFIG_HWFC_Disabled << UART_CONFIG_HWFC_Pos);
#endif
    NRF_UART0->BAUDRATE      = (UART_BAUDRATE_BAUDRATE_Baud115200 << UART_BAUDRATE_BAUDRATE_Pos);
    NRF_UART0->ENABLE        = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
    NRF_UART0->INTENSET      = (UART_INTENSET_RXDRDY_Msk |
                                UART_INTENSET_TXDRDY_Msk);

    NRF_UART0->EVENTS_RXDRDY = 0;
    NRF_UART0->EVENTS_TXDRDY = 0;
    NRF_UART0->TASKS_STARTRX = 1;
    NVIC_SetPriority(UART0_IRQn, 3);
    NVIC_EnableIRQ(UART0_IRQn);
}

void serial_handler_stop(void)
{
  NRF_UART0->EVENTS_RXTO = 0;

  NRF_UART0->TASKS_STOPTX = 1;
  NRF_UART0->TASKS_STOPRX = 1;

  // wait for receiver timeout
  while (! NRF_UART0->EVENTS_RXTO)
    ;

  NRF_UART0->ENABLE = (UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos);

  // Disable UART interrupt in NVIC
  NVIC_DisableIRQ(UART0_IRQn);

  // Disconnect high drive tx pin
  int tx_pin = NRF_UART0->PSELTXD;
  NRF_GPIO->PIN_CNF[tx_pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                            | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                            | (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos)
                            | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
                            | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);

  // Disconnect high drive rts pin
  int rts_pin = NRF_UART0->PSELRTS;
  NRF_GPIO->PIN_CNF[rts_pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                             | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                             | (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos)
                             | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
                             | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);

  m_serial_state = SERIAL_STATE_DISABLED;
}

uint32_t serial_handler_credit_available(void)
{
    return fifo_get_len(&m_rx_fifo);
}

void serial_wait_for_completion(void)
{
    uint32_t was_masked;
    _DISABLE_IRQS(was_masked);
    m_suspend = true;
    while (m_serial_state != SERIAL_STATE_IDLE)
    {
        UART0_IRQHandler();
    }
    m_suspend = false;
    _ENABLE_IRQS(was_masked);
}

bool serial_handler_event_send(serial_evt_t* evt)
{
  if (m_serial_state == SERIAL_STATE_DISABLED) {
      return false;
  }
    if (fifo_is_full(&m_tx_fifo))
    {
        return false;
    }

    serial_data_t raw_data;
    raw_data.status_byte = 0;
    uint8_t *buf = raw_data.buffer + 1;
    uint8_t *src = (uint8_t*)evt;
    uint8_t *end = src + evt->length + 1;
    *(buf++) = 0x71;
    while (src < end)
    {
        if ((*src & 0xf0) == 0x70)
        {
            *(buf++) = 0x72;
            *(buf++) = *(src++) ^ 0x20;
        }
        else
            *(buf++) = *(src++);
    }
    *(buf++) = 0x73;
    *raw_data.buffer = (buf - raw_data.buffer) - 2;
    fifo_push(&m_tx_fifo, &raw_data);

    if (m_serial_state == SERIAL_STATE_IDLE)
    {
        schedule_transmit();
    }

    return true;
}

bool serial_handler_command_get(serial_cmd_t* cmd)
{
    if (m_serial_state == SERIAL_STATE_DISABLED) {
        return false;
    }
    serial_data_t temp;
    if (fifo_pop(&m_rx_fifo, &temp) != NRF_SUCCESS)
    {
        return false;
    }
    if (((serial_cmd_t*) temp.buffer)->length > 0)
    {
        memcpy(cmd, temp.buffer, ((serial_cmd_t*) temp.buffer)->length + 1);
    }

    if (m_serial_state == SERIAL_STATE_WAIT_FOR_QUEUE)
    {
        m_serial_state = SERIAL_STATE_IDLE;
        NRF_UART0->TASKS_STARTRX = 1;
    }
    return true;
}
