# Long-Running Test Script

This directory contains a bash script for running long-running LODA tests with automatic Discord integration.

## Scripts

### `run_long_tests.sh`

A bash script that runs long-running LODA tests and uploads results to Discord.

**Tests included:**
- `test-inceval` - Incremental evaluator tests
- `test-pari` - PARI/GP formula tests
- `test-analyzer` - Program analyzer tests
- `test-lean` - Lean theorem prover formula tests

**Features:**
- Runs each test sequentially
- Captures output and exit status for each test
- Measures test execution time
- Uploads results to Discord immediately after each test completes
- Color-coded console output
- Automatic error handling

## Usage

### Basic Usage

```bash
./tests/run_long_tests.sh
```

### With Discord Integration

To enable Discord notifications, set the `LODA_DISCORD_WEBHOOK` environment variable:

```bash
export LODA_DISCORD_WEBHOOK='https://discord.com/api/webhooks/YOUR_WEBHOOK_ID/YOUR_WEBHOOK_TOKEN'
./tests/run_long_tests.sh
```

Alternatively, you can set it inline:

```bash
LODA_DISCORD_WEBHOOK='https://discord.com/api/webhooks/...' ./tests/run_long_tests.sh
```

### From GitHub Actions

The script can be integrated into GitHub Actions workflows:

```yaml
- name: Run Long Tests
  env:
    LODA_DISCORD_WEBHOOK: ${{ secrets.DISCORD_WEBHOOK }}
  run: ./tests/run_long_tests.sh
```

## Environment Variables

The script uses the same environment variable pattern as the LODA Log class:

- `LODA_DISCORD_WEBHOOK` - Discord webhook URL for uploading test results
  - If not set, the script will still run tests but skip Discord uploads
  - The webhook URL should be in the format: `https://discord.com/api/webhooks/{webhook.id}/{webhook.token}`

## Prerequisites

Before running the script, ensure:

1. LODA is built:
   ```bash
   cd src && make -f Makefile.linux-x86.mk
   ```

2. LODA home directory is set up:
   ```bash
   ./loda setup
   ```
   
   Or manually set up the required directories:
   ```bash
   mkdir -p $HOME/loda
   git clone https://github.com/loda-lang/loda-programs.git $HOME/loda/programs
   ```

3. For PARI tests, install PARI/GP:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install pari-gp
   
   # macOS
   brew install pari
   ```

## Output Format

The script provides detailed console output with:
- Test status (PASSED/FAILED)
- Exit code
- Execution duration
- Last 20 lines of test output

Discord messages include:
- Status emoji (✅ for pass, ❌ for fail)
- Test name
- Exit code
- Duration
- Last 20 lines of output (truncated to fit Discord's message limits)

## Example Output

```
==========================================
LODA Long-Running Tests
==========================================
LODA binary: /path/to/loda
Discord webhook: configured

==========================================
Running: test-inceval
Command: /path/to/loda test-inceval
==========================================

✓ Test passed (exit code: 0, duration: 45s)

Last 20 lines of output:
----------------------------------------
2025-11-03 18:20:29|INFO |Passed evaluation check for 146246 programs
----------------------------------------

✓ Results uploaded to Discord

==========================================
All tests completed!
==========================================
```

## Troubleshooting

**"loda binary not found"**
- Build LODA first: `cd src && make -f Makefile.linux-x86.mk`

**"Directory not found: /home/user/loda/"**
- Run `./loda setup` or manually create the LODA home directory

**Discord upload fails**
- Check that `LODA_DISCORD_WEBHOOK` is set correctly
- Verify the webhook URL is valid and accessible
- Check network connectivity

**Tests fail with external tool errors (PARI, Lean)**
- Install the required external tools (see Prerequisites)
- These tests are optional and can be skipped by commenting them out in the script
