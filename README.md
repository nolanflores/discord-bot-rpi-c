# Discord Bot - C Implementation

A Discord bot using Unix sockets to implement the WebSocket protocol and make HTTPS connections.
This bot connects to the Discord Gateway API and Discord REST API to read and respond to commands.
It was created to be run continuosly a Raspberry, but it can be run on other Unix systems.

## Disclaimer
I, the author nolanflores, use this bot for personal use. As such, I have named my bot "Cumbot" and implemented commands pertinent to me. You will likely want to parse through and edit the code to change your bot's name and functionality. Perhaps some day I will add a pre-processor variable for the name.

This also requires that you setup a bot account on the Discord dev website to obtain a Bot token. Extra authorization is also required if your bot becomes a member of more than 100 servers. It is your duty to make sure you are following Discord's terms of service.

### Prerequisites

This bot requires the following things in order to function

- GCC compiler with C11 support
- OpenSSL dev libraries
- A valid Discord Bot token

### Installing

#### Clone the Repository

```
git clone https://github.com/nolanflores/discord-bot-rpi-c.git
```
or use the SSH link
```
git clone git@github.com:nolanflores/discord-bot-rpi-c.git
```


#### Install the compiler and library

Install GCC (probably already installed on your system)

```
sudo apt install gcc
```

Install libssl-dev

```
sudo apt install libssl-dev
```

#### Insert your Bot token

Open the file "discord_token.h" in the project root and paste your bot token where it says "YOUR_BOT_TOKEN_HERE"
> [!WARNING]
> DO NOT PUSH YOUR BOT TOKEN TO YOUR REPOSITORY.
> It can result in outsiders having access to your bot.

## Running the bot

I have provided a script for compilation

```
./compile.sh
```

This will create an executable file that can be run

```
./run
```

## Deployment

I run this program with systemd in order to restart the program should it fail. It is not necessary, but I would recomend looking into it.

## Built With

* C11 - Programming language used
* OpenSSL - SSL/TLS implementation for secure HTTPS and WebSocket connections
* [cJSON](https://github.com/DaveGamble/cJSON) - JSON parsing library
* POSIX Threads - Heartbeat management

## Authors

* **Nolan Flores** - [nolanflores](https://github.com/nolanflores)

## License

This project is open source.

## Acknowledgments

* [Discord Gateway Documentation](https://discord.com/developers/docs/events/gateway)
* [Other Discord Documentation](https://discord.com/developers/docs/intro)
* [WebSocket Wikipedia Page](https://en.wikipedia.org/wiki/WebSocket)

## Author Remarks

This project is years in the making. I started on this path back in 2021 in my Sophmore year of high school. I was inspired by the Youtuber/Twitch streamer [DougDoug](https://www.youtube.com/@DougDoug), who had a series of videos where his Twitch chat was able to play the video game [Peggle](https://en.wikipedia.org/wiki/Peggle) via code he wrote. I was taking AP Computer Science Principles at the time, which was teaching Python, as well as was (and still am) an avid gamer that used Discord to communicate with my friends. I attempted to copy DougDoug's concept by writing Python code that would allow my friends to play Peggle by sending messages in a Discord chat. The concept was cool and all, but the most entertaining part was my friends controlling my keyboard via Discord. When I was gifted a Raspberry Pi 3B+ for my birthday, I decided to use it to make a bot. My original bot was written in Python, and it didn't have a whole lot of functionality. I ended up losing that code, as I do not know where I put the Micro SD card it was saved on.

The Project was shelved until I went to university, where the seperation of me and my high school friends inspired me to rewrite the bot. The second edition was written in Javascript and run via Nodejs. It was much more robust than the Python bot, and had way more features. Either due to the Pi it was run on, or bad code, or something else, some messages would take forever to get a response from the Bot, so I decided to attempt rewriting it in C++. I used the library [DPP](https://dpp.dev/) (Discord Plus Plus) which handled most of the functionality. I wanted a lower level control of what my bot was doing. It just so happened that my Operating Systems I class was covering [Berkley Sockets](https://en.wikipedia.org/wiki/Berkeley_sockets), so I decided to rerewrite my bot in C, so that I could have knowledge and control over what my bot was actually doing.

I now keep track of all three versions on Github. This repository is the C version. The Nodejs and C++ versions can be found at the following repositories.
- Javascript - https://github.com/nolanflores/discord-bot-rpi-javascript
- C++ - https://github.com/nolanflores/discord-bot-rpi-cpp