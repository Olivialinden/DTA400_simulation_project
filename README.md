# DTA400 Simulation Project: TCP NewReno Settings in Three Network Scenarios

This repository contains the code, results and measurements for the DTA400 project
report *Optimal TCP NewReno Socket Settings for Short Transfers in Home, Mobile and
Long-Distance Networks using ns-3* (Yi Chen, University West, 2026).

The simulation transfers a 1 MB file over one point-to-point link with TCP NewReno in
ns-3 and measures the transfer time. Three socket parameters are tested in all
3 × 3 × 3 = 27 combinations, in three scenarios:

| Parameter     | Values           |
|---------------|------------------|
| InitialCwnd   | 1, 4, 10         |
| SegmentSize   | 536, 1000, 1448  |
| DelAckCount   | 1, 2, 4          |

| Scenario      | Link speed | One-way delay | Packet loss |
|---------------|------------|---------------|-------------|
| Home          | 63 Mbps    | 10 ms         | 0           |
| Mobile (5G)   | 160 Mbps   | 16 ms         | 0.01%       |
| Long distance | 63 Mbps    | 180 ms        | 0.015%      |

## Files

| File | Description |
|------|-------------|
| `tcp-project.cc` | The ns-3 simulation program (based on the ns-3 tutorial `fifth.cc`). |
| `run-all.sh` | Runs all combinations and saves the results in `results.csv`. Can also produce the cwnd traces. |
| `results.csv` | Results of all 1647 runs (27 home + 810 mobile + 810 long distance). |
| `cwnd-files (1).tar.gz` | Congestion window traces (`cwnd-*.dat`) for the long-distance scenario without random loss. |
| `simple_figure.py` | Draws Fig. 1 of the report from `results.csv`. |
| `fig1-simple.pdf` | Fig. 1 of the report. |
| `measurements.docx` | Screenshots of the ping and speed-test measurements (18 October 2026) used for the scenarios. |

## Requirements

- ns-3 version 3.48
- bash
- Python 3 with matplotlib (`pip install matplotlib`) for the figure

## How to run

1. Copy the program into the `scratch` folder of ns-3 and build:

   ```bash
   cp tcp-project.cc <ns-3 folder>/scratch/
   cd <ns-3 folder>
   ./ns3 build
   ```

2. Run one simulation, for example the best setting in the long-distance scenario:

   ```bash
   ./ns3 run "tcp-project --dataRate=63Mbps --delay=180ms --errorRate=0.00015 \
              --initCwnd=10 --segmentSize=1448 --delAckCount=1 --RngRun=1"
   ```

   The program prints one line:
   `dataRate,delay,errorRate,initCwnd,segmentSize,delAckCount,run,receivedBytes,transferTime`

3. Run all combinations (copy `run-all.sh` to the ns-3 folder first). The results are saved in `results.csv`:

   ```bash
   bash run-all.sh                 # all 3 scenarios, 30 runs where there is random loss
   bash run-all.sh australia       # only one scenario (home, mobile or australia)
   NRUNS=5 OUT=test.csv bash run-all.sh mobile   # fewer runs, own output file
   ```

4. Produce the 7 congestion window traces (`cwnd-*.dat`):

   ```bash
   CWND=1 bash run-all.sh
   ```

5. Draw Fig. 1:

   ```bash
   python3 simple_figure.py results.csv
   ```

## Output formats

**`results.csv`**: one line per run.

| Column | Meaning |
|--------|---------|
| `scenario` | home, mobile or australia (long distance) |
| `dataRate`, `delay`, `errorRate` | link speed, one-way delay, packet loss probability |
| `initCwnd`, `segmentSize`, `delAckCount` | the three TCP parameters |
| `run` | random stream number (RngRun) |
| `receivedBytes` | bytes received (1000000 when the file is complete) |
| `transferTime` | time from the start of the sender to the last byte received, in seconds (-1 = not finished) |

**`cwnd-*.dat`**: two columns separated by a tab: simulation time (s) and congestion window (bytes).
The sender starts at 1.0 s. The file names give the setting:
`best` = (10, 1448, 1), `cw1`/`cw4` = InitialCwnd 1/4, `seg536`/`seg1000` = SegmentSize 536/1000,
`delack2`/`delack4` = DelAckCount 2/4. Extract them with `tar -xzf "cwnd-files (1).tar.gz"`.
