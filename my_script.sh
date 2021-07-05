#/bin/sh -v
#
# My test script to test my submission for sttyl.
# It removes any previous compilations, and re-compiles
# the program fresh.
#

#-------------------------------------
#    compile program
#-------------------------------------
make clean
make

# Run course-provided test script
~lib215/hw/smsh/test.smsh.19

# Test comment-handling
./smsh test_comments.sh > test_comments.out.smsh
dash test_comments.sh > test_comments.out.dash
diff test_comments.out.smsh test_comments.out.dash

rm test_comments.out.smsh test_comments.out.dash

if [ $? -eq 0 ]
then
    echo Correctly handled comments.
else
    echo Failed comment handling.
fi

# Test variable assignment (test #8 from course-script)
./smsh test_assign.sh
echo "Exit status is $?, expecting non-zero"
