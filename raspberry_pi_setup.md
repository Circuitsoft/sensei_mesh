#### Hardware

 * Raspberry Pi 3 Model B
 * Raspberry Pi Camera Module V2
 * 2.5A Power supply
 * Case

#### System Setup

 * Install latest Raspian
 * Configure keyboard, password using `sudo raspi-config`
 * Configure wifi by editing /etc/wpa_supplicant/wpa_supplicant.conf
 * Edit /etc/gai.conf to prefer ipv4

#### Dependencies install
 * Clone [sensei_client](git@github.com:WildflowerSchools/sensei_client.git), [camera](git@github.com:WildflowerSchools/camera.git), and [sensei_mesh](git@github.com:WildflowerSchools/sensei_mesh.git) repositories.
 * Install the sensei_client module: `sudo pip3 install -e .`, while in the sensei_client directory.
 * Update system with `sudo apt-get update && sudo apt-get upgrade`
 * Install packages: `git python3 python3-pip python pip`
 * `sudo pip3 install awscli picamera ipython pyserial boto3`

#### Configure

 * `aws configure`
 * Copy `pyaci/example_sensei.yaml` to `~/.sensei.yaml` and edit it to provide TC login, and path to serial device.
