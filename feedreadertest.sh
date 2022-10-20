#!/bin/bash

# Script for testing feedreader project
# Author: Vojtěch Dvořák (xdvora3o)

# Tests cases are located in folder described by TEST_DIR path

# Each test case has own folder with multiple files:
# TEST_FILE_NAME = file with arguments for specific test case 
#                  (or with description of test case on the first line and 
#                   arguments on the second line)
# OUPUT_FILE_NAME = file with expected output (stdout) of program
# RET_CODE_FILE_NAME = file with expected return code of program
#
# During run time is RESULT_FILE_NAME in test case folder generated 
# for real output to stdout and it is compared with expected result by diff
#
# If OUPUT_FILE_NAME or RET_CODE_FILE_NAME is missing, there is no comparison
# of expected return code or output (depends on missing file)
#
# For preserving output files (*.tmp) use -v option, otherwise thy are deleted

PROGRAM_PATH="./feedreader"

TEST_DIR="tests/*" # Path to directories with test files

TEST_FILE_NAME="test"
OUTPUT_FILE_NAME="out"
RET_CODE_FILE_NAME="ret"

RESULT_FILE_NAME="out.tmp" # File with STDOUT that was produced by the program
ERROR_FILE_NAME="err.tmp" # File with STDERR that was produced by the program
DIFF_FILE_NAME="diff.tmp" # Differences between expected and real STDOUT

VERBOSE=0 # Default value of 

PASSED_MSG="[ \033[0;32mPASSED\033[0m ]"
FAILED_MSG="[ \033[0;31mFAILED\033[0m ]"

# Parse options
while getopts "v" OPT
do
    if [ "$OPT" = "v" ]
    then
        VERBOSE=1
    fi
done



date
echo "Running tests of feedreader:"

RETURN_PATH=$(realpath .)
for TEST in $TEST_DIR
do
    if [ -d $TEST ]
    then
        echo -e -n "${TEST#$TEST_DIR}:\t"

        if [ -f "$TEST/$TEST_FILE_NAME" ]
        then
            HEAD_LINES=`head -2 "$TEST/$TEST_FILE_NAME"`

            INIT_LINE=`echo "$HEAD_LINES" | head -1`
            ARGS=$INIT_LINE # Arguments for program in this test case

            if [[ "${INIT_LINE:0:1}" == "#" ]]
            then
                DESCRIPTION=`echo "${INIT_LINE:1:60}" | xargs` # Remove # from start and trim string
                ARGS=`echo "$HEAD_LINES" | tail -1` # Line with test command is second line of test file
            else
                DESCRIPTION=""
            fi

            ERROR_FILE=$ERROR_FILE_NAME
            RESULT_FILE=$RESULT_FILE_NAME
            PROGRAM_REALPATH=$(realpath ${PROGRAM_PATH})
            cd $TEST # Go to Directory with current test
            eval "${PROGRAM_REALPATH} >${RESULT_FILE} 2>${ERROR_FILE} ${ARGS}" # Testing
            RETURN_CODE=$?
            
            REASON=""
            RESULT=$PASSED_MSG
            if [ -f "$RET_CODE_FILE_NAME" ]
            then
                read EXPECTED_RETURN_CODE < "$RET_CODE_FILE_NAME"

                if [ $EXPECTED_RETURN_CODE != $RETURN_CODE ]
                then
                    REASON="${REASON}Return code mismatch, Expected: $EXPECTED_RETURN_CODE Got: $RETURN_CODE\n"
                    RESULT=$FAILED_MSG
                fi
            fi

            if [ -f "$OUTPUT_FILE_NAME" ]
            then
                diff $RESULT_FILE $OUTPUT_FILE_NAME > $DIFF_FILE_NAME
                if [ $? != 0 ]
                then
                    REASON="${REASON}Different outputs! (use -v to preserve $RESULT_FILE_NAME file)\n"
                    RESULT=$FAILED_MSG
                fi
            fi

            if [ $VERBOSE != 1 ] # Remove temporary files
            then
                rm -f $ERROR_FILE $RESULT_FILE $DIFF_FILE_NAME
            fi

            echo -e "$RESULT\t$DESCRIPTION"
            if [[ $REASON != "" ]]
            then
                echo -e -n "Why: \t$REASON"
            fi

            cd $RETURN_PATH # Return to default directory
        else
            echo -e "\033[0;33mMandatory '$TEST_FILE_NAME' file in the test directory '$TEST' was not found!\033[0m"
        fi
    fi
done 
