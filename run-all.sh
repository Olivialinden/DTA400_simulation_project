#!/bin/bash
# DTA400 project: run every combination of settings and save the results.
#
# Usage (from the ns-3 top folder, after "./ns3 build"):
#     bash run-all.sh                # all 3 scenarios
#     bash run-all.sh australia      # only one scenario (for a quick test)
#     NRUNS=5 OUT=test.csv bash run-all.sh mobile   # fewer seeds, own file
#     CWND=1 bash run-all.sh         # only the 7 cwnd traces (cwnd-*.dat)
#
# Output: results.csv (one line per run)

# ---------------------------------------------------------------------------
# Scenarios: name  dataRate  one-way-delay  packetLossRate
#
# dataRate = speed test with Bredbandskollen (18 Oct 2026):
#              home Wi-Fi (2.4 GHz) about 63 Mbps, 5G mobile hotspot about 160 Mbps.
#              The long-distance scenario uses the same home connection.
# delay    = half of the AVERAGE round-trip time of 10 pings (18 Oct 2026):
#              home      : ping www.kth.se on home Wi-Fi         = 20 ms  -> 10 ms
#              mobile    : ping www.kth.se on 5G phone hotspot   = 32 ms  -> 16 ms
#              australia : ping mirror.aarnet.edu.au, home Wi-Fi = 360 ms -> 180 ms
# loss     = packet loss probability (per packet):
#              home      : 0 (short fixed path)
#              mobile    : 0.0001 = 0.01 %, median TCP loss in a commercial
#                          5G network (Narayanan et al., WWW 2020)
#              australia : 0.00015 = 0.015 %, within the < 0.1 % that PingER
#                          reports between Europe and Australasia
# ---------------------------------------------------------------------------
SCENARIOS=(
  "home       63Mbps    10ms   0"
  "mobile     160Mbps   16ms   0.0001"
  "australia  63Mbps    180ms  0.00015"
)

# The three parameters and their test values
INITCWND=(1 4 10)
SEGSIZE=(536 1000 1448)
DELACK=(1 2 4)

# ---------------------------------------------------------------------------
# Optional: only the cwnd traces (long-distance scenario, NO random loss).
# Best setting (10, 1448, 1), then one parameter changed at a time.
# ---------------------------------------------------------------------------
if [ "${CWND:-0}" = "1" ]; then
  # name  initCwnd  segmentSize  delAckCount
  TRACES=(
    "best      10  1448  1"
    "cw1       1   1448  1"
    "cw4       4   1448  1"
    "seg536    10  536   1"
    "seg1000   10  1000  1"
    "delack2   10  1448  2"
    "delack4   10  1448  4"
  )
  for t in "${TRACES[@]}"; do
    read -r name cw seg da <<< "$t"
    ./ns3 run --no-build "tcp-project --dataRate=63Mbps --delay=180ms --errorRate=0 \
           --initCwnd=$cw --segmentSize=$seg --delAckCount=$da --RngRun=1 \
           --cwndFile=cwnd-$name.dat" 2>/dev/null | tail -1
    echo "cwnd-$name.dat written"
  done
  exit 0
fi

# Repetitions with different random seeds (default 30; change with NRUNS=...)
NRUNS=${NRUNS:-30}
RUNS=($(seq 1 "$NRUNS"))

ONLY="$1"           # optional: run only this scenario
OUT=${OUT:-results.csv}   # change with OUT=other-file.csv

echo "scenario,dataRate,delay,errorRate,initCwnd,segmentSize,delAckCount,run,receivedBytes,transferTime" > "$OUT"

total=0
for s in "${SCENARIOS[@]}"; do
  read -r name rate delay err <<< "$s"
  if [ -n "$ONLY" ] && [ "$ONLY" != "$name" ]; then continue; fi
  # Without random loss every run gives the same result, so one run is enough
  if [ "$err" = "0" ]; then runs=(1); else runs=("${RUNS[@]}"); fi
  for cw in "${INITCWND[@]}"; do
    for seg in "${SEGSIZE[@]}"; do
      for da in "${DELACK[@]}"; do
        for run in "${runs[@]}"; do
          line=$(./ns3 run --no-build "tcp-project --dataRate=$rate --delay=$delay \
                 --errorRate=$err --initCwnd=$cw --segmentSize=$seg \
                 --delAckCount=$da --RngRun=$run" 2>/dev/null | tail -1)
          echo "$name,$line" >> "$OUT"
          total=$((total + 1))
          echo "[$total] $name cwnd=$cw seg=$seg delack=$da run=$run -> $line"
        done
      done
    done
  done
done

echo "Done: $total runs saved in $OUT"
