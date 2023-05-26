# Validator

This page comprises information for setting up a worker node in the validation mode. In this mode, `loda` fetches submitted programs from the [API server](https://github.com/loda-lang/loda-api), validates them and adds them to the [programs repository](https://github.com/loda-lang/loda-programs). It also checks and updates existing programs.

## Recommended VM Setup

* 2 vCPUs
* 2 GB main memory (1 GB per vCPU)
* 30 GB root disk
* Latest Ubuntu LTS image
* Pre-emptive ('spot') mode for cost saving (restarts are fine)
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

We recommend to access the LODA API server via an URL in the local network. You also may want to reduce the OEIS and programs update intervals to 1 day. For the recommended setup of 2 GB main memory, you should reduce the maximum physical memory per instance to 750 MB. Add these lines to `~/loda/setup.txt`:

```bash
LODA_API_SERVER=http://loda-api-1
LODA_GITHUB_UPDATE_INTERVAL=1
LODA_OEIS_UPDATE_INTERVAL=1
LODA_MAX_PHYSICAL_MEMORY=750
```

Install the `loda-validator.sh` script:

```bash
wget -qO ~/loda/bin/loda-validator.sh https://raw.githubusercontent.com/loda-lang/loda-cpp/master/validator/loda-validator.sh
chmod +x ~/loda/bin/loda-validator.sh
```

Verify the correct number of validator instances in `loda-validator.sh`:

```bash
# number of instances
num_new=1
num_update=1
```

Install the validator configuration file `~/loda/miners.json`:

```bash
wget -qO ~/loda/miners.json https://raw.githubusercontent.com/loda-lang/loda-cpp/master/validator/miners.json
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

Login to the [@lodaminer](https://twitter.com/lodaminer) Twitter account and authorize the app:

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

Generate an API token and update this line in `~/loda/bin/loda-validator.sh`:

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
alias tl="tail -f $HOME/validate-*.out"
alias lc="loda check -b"
alias le="loda eval -b -s"
```

Now it's time to start the validators:

```bash
loda-validator.sh
```

Already running validators are stopped and the new validators are started automatically in background mode. You can follow the logs using the `tl` alias.

## Cron Jobs

It is helpful to regularly restart the validators using cron jobs:

```crontab
30 1 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
30 3 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
30 5 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
30 7 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
30 9 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
30 13 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
30 15 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
30 18 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
30 21 * * * $HOME/loda/bin/loda-validator.sh > $HOME/loda-validator.out 2>&1
```

To automatically commit and push added and reviewed updated programs, add these jobs:

```crontab
50 18 * * * cd $HOME/loda/programs/scripts; ./update_programs.sh --commit-staged > $HOME/update_programs.out 2>&1
00 19 * * * cd $HOME/loda/programs/scripts; ./add_programs.sh > $HOME/add_programs.out 2>&1
```

Optional jobs for updating pattern file and program lists:

```
# 30 5 * * SUN $HOME/loda/bin/loda-patterns.sh > $HOME/patterns.out 2>&1
# 0 10 * * SUN cd $HOME/git/loda-lang.github.io; ./update-lists.sh > $HOME/lists.out 2>&1
```

## Hyperscaler Startup Script and Schedule

If you run on a hyperscaler, you can add this startup script to your VM definition. This is particularly useful if you run in pre-emptive mode.

```bash
sudo -u <USERNAME> -H bash /home/<USERNAME>/loda/bin/loda-validator.sh
```

You may also want to create an instance schedule to start and stop the VM regularly.

## Operations

New programs are automatically added, commited and pushed to the programs repository using the `add_programs.sh` script that is scheduled in the cron job. Updated and deleted programs need to be reviewed though. There are interactive shell scripts available:

```bash
cd ~/loda/programs/scripts

# review and stage updated programs:
./update_programs.sh

# review and stage deleted programs:
./delete_programs.sh
```

As mentioned already, if you have set up the cron jobs as described above, you don't need to care about adding programs manually. Also note that the cron jobs will take care of committing and pushing the updated and deleted programs as well. Hence it is sufficient to stage updated and deleted programs using the scripts but not commit them directly. The scripts detect is a change is staged already or not. You can interrupt and restart program reviews at any point in time.

To update the history chart, run these commands:

```bash
sudo apt install gnuplot-nox
cd loda/programs/scripts/
./make_charts.sh
cd ..
git add program_counts.png
```
