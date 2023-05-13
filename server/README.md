# Validator

This page comprises information for setting up a worker node in the validation mode.
In this mode, LODA fetches submitted programs from the API server, validates them
and ads them to the programs repository. It also checks and updates existing programs.

## Recommended VM Setup

* 2 vCPUs
* 2 GB main memory (reserve 1 GB per vCPU)
* Pre-emptive ('spot') mode for cost saving (restarts are fine)
* 30 GB root disk
* Latest Ubuntu LTS image
* SSH access only (no HTTP/HTTPS access)

## GitHub Access

Install git:

```bash
sudo apt update
sudo apt install git
```

Create a key pair:

```bash
ssh-keygen -t rsa
cat ~/.ssh/id_rsa.pub
```

Login to GitHub using the `loda-bot` account and add the public key to its [SSH keys](https://github.com/settings/keys).

Set the local git user name and e-mail address (replace proper address!):

```bash
git config --global user.name "LODA Bot"
git config --global user.email "..."
```

## Install LODA

Follow the [installation instructions](https://loda-lang.org/install/) with these details:

* Default install directory
* Programs repository URL:
 `git@github.com:loda-lang/loda-programs.git`
* Choose `server` mode

We recommend to access the LODA API server via an URL in the local network. You also may want to reduce the data update intervals to 1 day.
For the recommend setup of 2 GB main memory, you
should reduce the maximum physical memory to 750 MB.
Add these lines to `~/loda/setup.txt`:

```bash
LODA_API_SERVER=http://loda-api-1
LODA_GITHUB_UPDATE_INTERVAL=1
LODA_OEIS_UPDATE_INTERVAL=1
LODA_MAX_PHYSICAL_MEMORY=750
```

Install the `loda-server.sh` script:

```bash
wget -qO ~/loda/bin/loda-server.sh https://raw.githubusercontent.com/loda-lang/loda-cpp/master/server/loda-server.sh
chmod +x ~/loda/bin/loda-server.sh
```

Verify the correct number of validator instances in `loda-server.sh`:

```bash
# number of instances
num_new=1
num_update=1
```

Install the validator configuration `~/loda/miners.json` usig these commands:

```bash
wget -qO ~/loda/miners.json https://raw.githubusercontent.com/loda-lang/loda-cpp/master/server/miners.json
```

## Metrics

To send metrics to InfluxDB, add the host and credential information to `~/loda/setup.txt`:

```bash
LODA_INFLUXDB_AUTH=<user>:<pw>
LODA_INFLUXDB_HOST=http://loda-api-1/influxdb
LODA_METRICS_PUBLISH_INTERVAL=60
```

## Twitter Notifications

Install the [Rainbow stream](https://github.com/orakaro/rainbowstream) command-line client:

```bash
sudo apt install python3-pip
sudo pip3 install rainbowstream
```

Login to the `@lodaminer` Twitter account and authorize the app:

```bash
rainbowstream
```

Add this line to `~/loda/setup.txt`:

```bash
LODA_TWEET_ALERTS=yes
```

## Slack Notifications

Install Slack command-line client:

```bash
sudo apt install jq
wget -qO ~/loda/bin/slack https://raw.githubusercontent.com/rockymadden/slack-cli/master/src/slack
chmod +x ~/loda/bin/slack
```

Generate an API token and update this line in `~/loda/bin/loda-server.sh`:

```bash
export SLACK_CLI_TOKEN=...
```

Enable Slack alerts in `~/loda/setup.txt`:

```bash
LODA_SLACK_ALERTS=yes
```

## Start Validators

Before starting the validators, defining these aliases can be useful:

```bash
alias tl="tail -f $HOME/server-*.out"
alias lc="loda check -b"
alias le="loda eval -b -s"
```

Now it's time to start the validators:

```bash
loda-server.sh
```

Already running validators and the new validators are started automatically in background mode.
You can follow the logs using the `tl` alias.

## Cron Jobs

It is helpful to regularly restart the validators using cron jobs:

```txt
30 1 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1
30 3 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1
30 5 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1
30 7 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1
30 9 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1
30 13 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1
30 15 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1
30 18 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1
30 21 * * * $HOME/loda/bin/loda-server.sh > $HOME/loda-server.out 2>&1

50 18 * * * cd $HOME/loda/programs/scripts; ./update_programs.sh --commit-staged > $HOME/update_programs.out 2>&1
00 19 * * * cd $HOME/loda/programs/scripts; ./add_programs.sh > $HOME/add_programs.out 2>&1

# 30 5 * * SUN $HOME/loda/bin/loda-patterns.sh > $HOME/patterns.out 2>&1
# 0 10 * * SUN cd $HOME/git/loda-lang.github.io; ./update-lists.sh > $HOME/lists.out 2>&1
```

## Hyperscaler Startup Script and Schedule

If you run on a hyperscaler, you can add this startup script. This is particularly useful if you run in pre-emptive mode.

```bash
export LODA_USER=...
sudo -u $LODA_USER -H bash /home/$LODA_USER/loda/bin/loda-server.sh
```

You may also want to create an instance schedule to start the VM automatically.
