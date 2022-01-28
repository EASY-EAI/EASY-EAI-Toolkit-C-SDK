#/bin/bash

set -e

api_list=`ls -I "build*" -I "output"`

usage()
{
	echo "DESCRIPTION"
	echo "EASYAI-1109_1126 SDK compiler tool."
	echo " "
	echo "SYNOPSIS"
	echo "	./builsh <api directory>"
	echo " "
	echo "OPTIONS"
	echo "	<api directory>	api directory name or 'all'"

}

compiler_api()
{
	# compiler
	if [ -e ../$1/CMakeLists.txt ]; then
		cmake ../$1
		make
	fi
	cd -

	# output
	rm output/$1 -rf
	mkdir -p output/$1

	if [ -e $1/CMakeLists.txt ]||[ -d $1/include ]||[ -d $1/src ]; then
		mv build/*.a output/$1
		if [ -e $1/libs ]; then
			cp $1/libs/* output/$1 -rf
		fi
		cp $1/include/* output/$1 -rf
	else
		cp $1/*.a output/$1 -rf
		cp $1/*.h output/$1 -rf
	fi
}


main() {
	if [ $# != 1 ]; then
		usage
		return 1
	fi

	if [ $1 = "all" ]; then
		
		for var in ${api_list[@]}
		do
			rm build -rf
			mkdir build
			cd build
			compiler_api $var
		done
	elif [ $1 = "clear" ]; then
		rm build -rf
		rm output -rf
	else
		rm build -rf
		mkdir build
		cd build
		compiler_api $1
	fi
}


main "$@"
