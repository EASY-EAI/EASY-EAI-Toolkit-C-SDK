#/bin/bash

set -e

# 前置库列表：需要被后面的库依赖，因此这些库需要提前编译
preLib_list="frame_queue"

# 后置库列表：需要依赖前置的库，因此编译顺序靠后
postLib_list=`ls -I "build*" -I "output" -I "frame_queue"`

api_list="$preLib_list $postLib_list"

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
#		cp $1/*.a output/$1 -rf
#		cp $1/*.h output/$1 -rf
		cp $1/* output/$1 -rf
	fi

	if [ -e $1/Readme ]; then
		cat $1/Readme
		if [ -e $1/sync.sh ]; then
		       	$1/sync.sh 
		fi
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
