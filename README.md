# Discord Bot - C Implementation

A Discord bot using Unix sockets to implement the WebSocket protocol and make HTTPS connections.
This bot connects to the Discord Gateway API and Discord REST API to read and respond to commands.
It was created to be run continuosly a Raspberry, but it can be run on other Unix systems.

## Disclaimer
I, the author nolanflores, use this bot for personal use. It is a pet project of mine, therefore I have implemented commands useful to me. You will likely want to parse through and tweak the code for yourself, such as changing the gifs the bot can send. Someday I would like to make it easier to change command names and actions.

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

This project is years in the making. I started on this path back in 2021 in my Sophmore year of high school. I was inspired by the Youtuber/Twitch streamer [DougDoug](https://www.youtube.com/@DougDoug), who had a series of videos where his Twitch chat was able to play the video game [Peggle](https://en.wikipedia.org/wiki/Peggle) via code he wrote. At the time, I was taking AP Computer Science Principles, which teaches Python. I also was playing a lot of video with friends over Discord, so I attempted to copy DougDoug's concept by writing Python code that would trigger keyboard and mouse events, allowing my friends to play Peggle on my computer by sending Discord messages. The concept was cool and all, but the most entertaining part was my friends controlling my keyboard to send messages from my Discord account. Kind of like a psuedo echo bot. When I was gifted a Raspberry Pi 3B+ for my birthday, I decided to use it to make a real bot. That original bot was written in Python, and it didn't have a whole lot of functionality. That bot died when I misplaced the Micro SD card that the original bot was on.

The Project was shelved until I went to university, where the seperation of myself andng  my high school friends inspired me to rewrite the bot. The second edition was written in Javascript and run via Nodejs. It was much more robust than the Python bot, and had way more features. Either due to the Pi it was run on, or bad code, or something else, some messages would take forever to get a response from the Bot, so I decided to attempt rewriting it in C++. I used the library [DPP](https://dpp.dev/) (Discord Plus Plus) which handled most of the functionality. I wanted a lower level control of what my bot was doing. It just so happened that my Operating Systems I class was covering [Berkley Sockets](https://en.wikipedia.org/wiki/Berkeley_sockets), so I decided to rerewrite my bot in C, so that I could have knowledge and control over what my bot was actually doing.

I now keep track of all three versions on Github. This repository is the C version. The Nodejs and C++ versions can be found at the following repositories.
- Javascript - https://github.com/nolanflores/discord-bot-rpi-javascript
- C++ - https://github.com/nolanflores/discord-bot-rpi-cpp