cmake -B build
# if [ $# -lt 1 ]; then
#     echo "매개변수가 안들어옴 ON 또는 OFF 기입"
#     exit
# fi

# COMPILE_EXECUTABLE=$1

# if  [ $COMPILE_EXECUTABLE == "ON" ]; then
#     cmake -B build -DCOMPILE_EXECUTABLE=ON
#     echo "Configured With COMPILE_EXECUTABLE=ON"
# elif [ $COMPILE_EXECUTABLE == "OFF" ]; then
#     cmake -B build
#     echo "Configured With COMPILE_EXECUTABLE=OFF"
# fi
