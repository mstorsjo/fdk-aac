# Benchmark tests

This Benchmark app analyses the time taken by AAC Encoder for given set of inputs. It is used to benchmark encoder module on android devices.

This page describes steps to run the AAC encoder Benchmark test.

Run the following steps to build the test suite:
```
mmm external/aac/tests/AacEncBenchmark/
```

# Resources
The resource folder for the tests is taken from [here](https://dl.google.com/android-unittest/media/external/aac/tests/AacEncBenchmark/AacEncBenchmark-1.0.zip)

Download the AacEncBenchmark-1.0.zip folder, unzip and push it to any path on the device, Let's say the path be /sdcard/test. You can give the path wherever you chose to put the files.

```
unzip AacEncBenchmark-1.0.zip
adb push AacEncBenchmark-1.0 /sdcard/test
```

# <a name="BenchmarkApplication"></a> Benchmark Application
To run the test suite for measuring performance, follow the following steps:

Benchmark Application can be run in two ways.

## Steps to run with atest
Note that atest command will install Benchmark application and push the required test files to the device as well.

For running the benchmark test, run the following command
```
atest AacEncBenchmark
```

## Steps to run without atest (push binary to the device and run)

To run the test suite for measuring performance of the encoder, follow the following steps:

The 64-bit binaries will be created in the following path : ${OUT}/data/benchmarktest64/AacEncBenchmark/

The 32-bit binaries will be created in the following path : ${OUT}/data/benchmarktest/AacEncBenchmark/

To test 64-bit binary push binaries from benchmarktest64.

```
adb push $(OUT)/data/benchmarktest64/AacEncBenchmark/AacEncBenchmark /data/local/tmp/
```

To test 32-bit binary push binaries from benchmarktest.

```
adb push $(OUT)/data/benchmarktest/AacEncBenchmark/AacEncBenchmark /data/local/tmp/
```

To get the resource files for the test follow instructions given in [Resources](#Resources)

After running the above steps, /sdcard/test should contain AacEncBenchmark-1.0 folder and /data/local/tmp should contain benchmark binary to be executed.

Run the following commands to see the benchmark results
```
adb shell
cd /data/local/tmp/
chmod a+x AacEncBenchmark
./AacEncBenchmark
```

Run the below commands to generate a csv report and see the benchmark results
```
adb shell
cd /data/local/tmp/
chmod a+x AacEncBenchmark
./AacEncBenchmark --benchmark_out=output.csv
```

if the folder path where the resource files are pushed is different from /sdcard/test/ , pass the actual folder path as an argument as shown below and run the following commands to see the benchmark results. Here let's say the path be /sdcard/test/AacEncBenchmark-1.0
```
adb shell
cd /data/local/tmp/
chmod a+x AacEncBenchmark
./AacEncBenchmark --path=/sdcard/test/AacEncBenchmark-1.0
```

Run the below commands to store the benchmark results in an output.csv file which will be generated in the same path on the device.

You can modify the output csv filename to any name and can be generated in any given absolute path.
```
adb shell
cd /data/local/tmp/
chmod a+x AacEncBenchmark
./AacEncBenchmark --path=/sdcard/test/AacEncBenchmark-1.0/ --benchmark_out=output.csv
```

# Analysis

The benchmark results are stored in a CSV file if opted, which can be used for analysis.

Note: This timestamp is in nano seconds and will change based on current system time.

This csv file can be pulled from the device using "adb pull" command.
```
adb pull /data/local/tmp/output.csv ./output.csv
```

## CSV Columns
Following columns are available in CSV.

Note: All time values are in nano seconds

1. **fileName**: The file being used as an input for the benchmark test.

2. **Channels**: Number of channels does the input audio bitstream contain.

3. **SampleRate**: SampleRate of the input audio bitstream.

4. **bitRate**: bitRate of the input audio bitstream.

5. **profile**: profile of the input audio bitstream.

6. **real_time**: Measures total elapsed time  from start to end of process, including wait times and delays.

7. **cpu_time**: Measures total time spent by cpu actively executing instructions for a process.
