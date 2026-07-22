# BeanChat Server Installation Guide

This guide explains how to install and run the **BeanChat Server** on Ubuntu using **systemd**.

---

# 0. Requirement
- Ubuntu 24.04 or newer
- a VPS or port forwarded system with Root or sudo access

---

# 1. Update the system

```bash
sudo apt update
```

---

# 2. Install required packages

BeanChat Server uses Qt for image processing (such as generating rounded avatars), so the following OpenGL libraries are required:

```bash
sudo apt install -y \
    libgl1 \
    libglx0 \
    libegl1 \
    libopengl0
```

Install UFW (firewall) if it is not already installed:

```bash
sudo apt install -y ufw
```

---

# 3. Open the firewall ports

```bash
sudo ufw allow 9987/tcp
sudo ufw allow 9987/udp
sudo ufw allow 20/tcp
```

> [!WARNING]
> please make sure open your ssh's port in ufw firewall
> 
> by default it's 20/tcp if it's differ replace 20 with your ssh your port

Verify the firewall rules:

```bash
sudo ufw status
```


Enable the firewall (if it is not already enabled):
```bash
sudo ufw enable
```

---

# 4. Create a dedicated server user

Running the server as its own user improves security.

```bash
sudo useradd -r -m -s /usr/sbin/nologin voicechat
```

---

# 5. Create the installation directory

```bash
sudo mkdir -p /home/voicechat/server
```

---

# 6. Download and Extract the server package

Download server from [BeanChat.ir](https://beanchat.ir/bc/#server)

Extract downloaded file, for example:

```bash
tar -xf BeanChatServer-0.x.x-linux.tar.gz
```

Rename the extracted directory:

```bash
mv BeanChatServer-0.x.x deployed
```

Copy it to the installation directory:

```bash
sudo cp -r deployed /home/voicechat/server/
```

---

# 7. Set file permissions

```bash
sudo chown -R voicechat:voicechat /home/voicechat/server
```

---

# 8. Create the systemd service

Create the service file:

```bash
sudo nano /etc/systemd/system/voicechat.service
```

Paste the following:

```ini
[Unit]
Description=BeanChat Server
After=network.target

[Service]
Type=simple
User=voicechat
WorkingDirectory=/home/voicechat/server/deployed/bin
ExecStart=/home/voicechat/server/deployed/bin/BeanChatServer
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```
> [!TIP]
>
> if you don't want run over port 9987 (which is default) you should pass it to BeanChatServer
>
> e.g in above instead of ExecStart=/home/voicechat/server/deployed/bin/BeanChatServer
>
> do:
> ExecStart=/home/voicechat/server/deployed/bin/BeanChatServer 9999
>
> also don't forget to open that port 9999 or any port you entered.

Save and close the editor. (press: CTL+X then: press Y then ENTER)

---

# 9. Start the server

Reload systemd:

```bash
sudo systemctl daemon-reload
```

Enable automatic startup:

```bash
sudo systemctl enable voicechat
```

Start the server:

```bash
sudo systemctl start voicechat
```

---

# Server Management

### Check server status

```bash
sudo systemctl status voicechat
```

### Restart the server

```bash
sudo systemctl restart voicechat
```

### Stop the server

```bash
sudo systemctl stop voicechat
```

### Start the server

```bash
sudo systemctl start voicechat
```

---

# Viewing Logs

View live logs:

```bash
journalctl -u voicechat -f
```

View today's logs:

```bash
journalctl -u voicechat --since today
```

---

# Updating BeanChat Server

Stop the server:

```bash
sudo systemctl stop voicechat
```

Replace the contents of the `deployed` directory with the new release.
> [!CAUTION]
>
> don't remove/replace directory: (avatar)
>
> and database file: (server.db)


Restore permissions:

```bash
sudo chown -R voicechat:voicechat /home/voicechat/server
```

Start the server again:

```bash
sudo systemctl start voicechat
```

---

# Default Ports

| Protocol | Port | Purpose |
|----------|------|---------|
| TCP | 9987 | Client connections |
| UDP | 9987 | Voice and video traffic |
| TCP | 20 | assuming your SSH port |

---

# Troubleshooting

Check whether the service is running:

```bash
sudo systemctl status voicechat
```

Follow the logs:

```bash
journalctl -u voicechat -f
```

If clients cannot connect:
> [!IMPORTANT]
>
> Verify ports **9987/TCP** and **9987/UDP** are open.
>
> Check your cloud provider's firewall/security group.
>
> Make sure the BeanChat service is running.
>
> If hosting at home, ensure your router forwards the required ports.
>
> if you had any problem with running contact us: support@beanchat.ir
