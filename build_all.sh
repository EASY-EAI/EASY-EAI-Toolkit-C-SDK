#/bin/bash

set -e

SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)
cd $SHELL_FOLDER

tar_name="easyeai-api"


# 前置库列表：需要被后面的库依赖，因此这些库需要提前编译
preLib_list="common_api"
# 后置库列表：需要依赖前置的库，因此编译顺序靠后
postLib_list=`ls -I "$tar_name*" -I "build_all.sh" -I "encryption" -I "LICENSE" -I "README*.md" -I "common_api"`

api_list="$preLib_list $postLib_list"


usage()
{
	echo "DESCRIPTION"
	echo "EASYAI-1109_1126 SDK compiler tool."
	echo " "
	echo "SYNOPSIS"
	echo "	./build_all.sh [clear]"
}

clear_api()
{
	for var in ${api_list[@]}
	do
		if [ -e $var/build.sh ]; then
			cd $var
			./build.sh clear
			cd - > /dev/null
		fi
	done
}

clear_sdk()
{
	rm $tar_name.tar.gz
	rm $tar_name -rf
}

main() {
	rm -rf "$tar_name"*

	if [ $# -eq 1 ]; then
		if [ $1 == "clear" ]; then
			clear_api
#			clear_sdk
		else
			usage
		fi

		exit 0
	fi

	mkdir "$tar_name"
	for var in ${api_list[@]}
	do
		if [ -e $var/build.sh ]; then
			cd $var
			./build.sh clear
			./build.sh all
			cd - > /dev/null

			if [ -d $var/output ]; then
				mkdir -p $tar_name/$var/
				cp -rf $var/output/* $tar_name/$var
			fi
		else
			cp $var/*.a $var/*.h $tar_name/$var
		fi
	done

	chmod 755 $tar_name -R
	
	tar czvf $tar_name.tar.gz $tar_name
	chmod 755 $tar_name.tar.gz

	ls -lh $tar_name.tar.gz
}


main "$@"
