cd $HOME/MNC2/dmadappa/scripts

./run_experiments -m1000 -l0.2 -c0.2 -t50 -p../sr -w10
./run_experiments -m1000 -l0.2 -c0.2 -t50 -p../sr -w50
./run_experiments -m1000 -l0.2 -c0.2 -t50 -p../sr -w100
./run_experiments -m1000 -l0.2 -c0.2 -t50 -p../sr -w200
./run_experiments -m1000 -l0.2 -c0.2 -t50 -p../sr -w500

./run_experiments -m1000 -l0.5 -c0.2 -t50 -p../sr -w10
./run_experiments -m1000 -l0.5 -c0.2 -t50 -p../sr -w50
./run_experiments -m1000 -l0.5 -c0.2 -t50 -p../sr -w100
./run_experiments -m1000 -l0.5 -c0.2 -t50 -p../sr -w200
./run_experiments -m1000 -l0.5 -c0.2 -t50 -p../sr -w500

./run_experiments -m1000 -l0.8 -c0.2 -t50 -p../sr -w10
./run_experiments -m1000 -l0.8 -c0.2 -t50 -p../sr -w50
./run_experiments -m1000 -l0.8 -c0.2 -t50 -p../sr -w100
./run_experiments -m1000 -l0.8 -c0.2 -t50 -p../sr -w200
./run_experiments -m1000 -l0.8 -c0.2 -t50 -p../sr -w500
