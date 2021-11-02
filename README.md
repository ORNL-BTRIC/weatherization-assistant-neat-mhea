## Weatherization Assistant NEAT/MHEA

This is the C/C++ calculation engine for NEAT/MHEA in the Weatherization
Assistant.

## Contributors
Michael Gettings  
Mark Fishbaugher  
Gina Accawi  
Brandon Langley  
Jason DeGraw

## Development Environment
Uses the clang compiler  
Uses simple make rather than cmake  
Uses clang-analyser 279 for code beautification and static analysis  
Uses getop() command line processing  
Uses (optionally, however builtin unit tests require ajv-cli that supports the draft7 version of json schema) npm ajv-cli presumed in the path, see everit-org in the wa_service for json schema validation upstream in API  

## Make Targets
See the documentation in the Makefile in the root of the repository for notes
on the various build targets and utilities implemented with make targets  

Basic build targets are:  

make clean  

This cleans up all of the outputs except the baseline folder  

make  

With no target specified the make utility just builds the wa_engine executable in the bin path which is a combination of both NEAT and MHEA engines.  Run ./bin/wa_engine --h for command line helps.

make neat_test  
make mhea_test  

Make the executable and then run all of the sample audits AND the regression test audit for NEAT or MHEA.  The comparison of the output with the baseline audits is contained in the output/report.base.* files for input, diagnostic, and output comparisons.  Any deviation (non zero fill diffs) should be investigated as a possible regression at each commit.  This target does NOT stop on regression delta, but rather continues on reporting all such deltas in the output/report.base.* files  

make neat_baseline  
make mhea_baseline  

Simply copies the last set of regression test outputs to the output/*/baseline path for NEAT or MHEA.  Run this after an expected change in baseline output so future regression test work with zero deltas.

## Distribution
The repo contains a .gitlab-ci.yml file containing distribution scripts.

## Setting a Version Number

When necessary to update the version number, there are several steps that should be taken.
1. You will need to update the tag in git. Running these commands should show you the list of tags:
   1. `git fetch -t`
   2. `git describe --tags`
2. Knowing the latest tag is important to determine what the new version number should be. The format of the version number is as follows: **vXX.YY.BBB** where XX is the 2 digit major version, YY is the two digit minor version, and BBB is the three digit build number. Minor revision number increment should be done when breaking changes are introduced. Otherwise, increment the build number. 
3. In the repository, there is a script to simplify this process. See bat/show_version.


Prior to distribution the following check_all bash script should be run which automates several make file calls with the correct parameters to validate regression tests:

bin/check_all
