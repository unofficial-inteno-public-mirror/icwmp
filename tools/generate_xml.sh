#!/bin/sh


# USAGE:
# ./generate_xml.sh <scripts path> <product class> <device protocol> <model name> <software version>
# If the input arguments are empty, then use the default values:
SCRIPTS_PATH=${1:-"/home/anis/cwmp/iop-aa/package/freecwmp/src/scripts/functions/"}
PRODUCT_CLASS=${2:-"DG301-W7P2U"}
DEVICE_PROTOCOL=${3:-"DEVICE_PROTOCOL_DSLFTR069v1"}
MODEL_NAME=${4:-"DG301-W7P2U"}
SOFTWARE_VERSION=${5:-"1.2.3.4B"}
cnt_obj=0
cnt_param=0

# Funtcions
# Check if object of parameters or get none in case of object contains an intance
is_object() {
	str=$1
	function=`echo $str |cut -d, -f1`
	if [ "$function" == "get_object_cache_generic" ]; then
		check=`echo $str |grep "\.[012345689]*\.,\$\|\.#\w*\.,\$"`
		if [ "$check" == "" ]; then
			echo "object"
		else
			echo "instance"
		fi
	else
		echo "parameter"
	fi
}

#	Check if object contains list of instances
is_array() {
	object=$1
	check=`cat script_list.txt |grep "$object[012345689]*\.,\$\|$object#\w*\.,\$"`
	if [ "$check" == "" ]; then
		echo "false"
	else
		echo "true"
	fi
}

get_param_type() {
	str=$1
	type=`echo $str |cut -d, -f3`
	type=${type#*xsd:}
	type=${type:-string}
	echo $type
}

xml_open_tag_object() {
	local objn="$1"
	local isarray="$2"
	local level="$3"
	local h_child="$4"
	local sp1=0 sp2=0
	let sp1=8+4*$level
	let sp2=$sp1+4
	printf "%${sp1}s"; echo "<parameter>"
	printf "%${sp2}s"; echo "<parameterName>$objn</parameterName>"
	printf "%${sp2}s"; echo "<parameterType>object</parameterType>"
	printf "%${sp2}s"; echo "<array>$isarray</array>"
	if [ -n "$h_child" -a "$h_child" != "0" ]; then
		printf "%${sp2}s"; echo "<parameters>"
	fi
}

xml_close_tag_object() {
	local level="$1"
	local h_child="$2"
	local sp1=0 sp2=0
	let sp1=8+4*$level
	let sp2=$sp1+4
	if [ -n "$h_child" -a "$h_child" != "0" ]; then
		printf "%${sp2}s"; echo "</parameters>"
	fi
	printf "%${sp1}s"; echo "</parameter>"
}

xml_add_parameter() {
	local paramn="$1"
	local type="$2"
	local level="$3"
	local sp1=0 sp2=0
	let sp1=8+4*$level
	let sp2=$sp1+4

	printf "%${sp1}s"; echo "<parameter>"
	printf "%${sp2}s"; echo "<parameterName>$paramn</parameterName>"
	printf "%${sp2}s"; echo "<parameterType>$type</parameterType>"
	printf "%${sp1}s"; echo "</parameter>"
}


xml_write_line() {
	local level="$1"
	local parent="$2"
	local path="$3"
	local line=""
	
	local LINES=`grep "$path[^,]\+$\|$path[^,]\+,$" tmp.txt`

	for line in $LINES; do
		local p=`echo "$line" | cut -d, -f$((level+2))`
		[ "$p" != "$parent" ] && continue
		local param=`echo "$line" | cut -d, -f$((level+3))`
		[ "$param" = "" ] && continue
		local node=`echo "$line" | cut -d, -f1`
		if [ "$node" = "object" ]; then
			local isarray=`echo "$line" | cut -d, -f2`
			let cnt_obj++
			local has_child=`grep "$path$param,[a-zA-Z0-9_,]\+$" tmp.txt |wc -l`;
			xml_open_tag_object "$param" "$isarray" "$level" "$has_child"
			xml_write_line "$((level+1))" "$param" "$path$param,"
			xml_close_tag_object "$level" "$has_child"
		elif [ "$node" = "parameter" ]; then
			local type=`echo "$line" | cut -d, -f2`
			let cnt_param++
			xml_add_parameter "$param" "$type" "$level"
		fi
	done
}

gen_data_model_xml_file() {
echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
echo "<deviceType xmlns=\"urn:dslforum-org:hdm-0-0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"urn:dslforum-org:hdm-0-0 deviceType.xsd\">"
echo "    <protocol>$DEVICE_PROTOCOL</protocol>"
echo "    <manufacturer>Inteno</manufacturer>"
echo "    <manufacturerOUI>002207</manufacturerOUI>"
echo "    <productClass>$PRODUCT_CLASS</productClass>"
echo "    <modelName>$MODEL_NAME</modelName>" 
echo "    <softwareVersion>$SOFTWARE_VERSION</softwareVersion>"
echo "    <dataModel>"
echo "        <attributes>"
echo "            <attribute>"
echo "                <attributeName>notification</attributeName>"
echo "                <attributeType>int</attributeType>"
echo "                <minValue>0</minValue>"
echo "                <maxValue>2</maxValue>"
echo "            </attribute>"
echo "            <attribute>"
echo "                <attributeName>accessList</attributeName>"
echo "                <attributeType>string</attributeType>"
echo "                <array>true</array>"
echo "                <attributeLength>64</attributeLength>"
echo "            </attribute>"
echo "            <attribute>"
echo "                <attributeName>visibility</attributeName>"
echo "                <attributeType>string</attributeType>"
echo "                <array>true</array>"
echo "                <attributeLength>64</attributeLength>"
echo "            </attribute>"
echo "        </attributes>"
echo "        <parameters>"
xml_write_line "1" "root" "root,"
echo "        </parameters>"
echo "    </dataModel>"
echo "</deviceType>"
}

echo "Start Generation of inteno.xml"

EXEC_PATH=`pwd`
SCRIPTS_LIST_FILE="$EXEC_PATH/script_list.txt"
# Extract object and parameter list from scripts
list=`ls $SCRIPTS_PATH |grep -v "common"`
cd $SCRIPTS_PATH
cat `echo $list` | grep "get_param_cache_generic \|get_object_cache_generic " |grep -v "^\\s*#" |awk -F '"' '{type=""; for (i = 3; i <= NF; i++) { if($i ~ /^xsd:/) { type = $i; break; } } print $1"  "$2"  "type; }' | awk '{print $1","$2","$3}' | sort -t, -k2 | sed -e "s|.\\$|.#|g" > $SCRIPTS_LIST_FILE

cat `echo $list` | grep "freecwmp_cache_output" |grep -v "^\\s*#" | awk -F '"' '{type=""; for (i = 3; i <= NF; i++) { if($i ~ /^xsd:/) { type = $i; break; } } print $2"  "type; }' | awk '{print ($1 ~ /\.$/ ? "get_object_cache_generic" : "get_param_cache_generic"), $0}'| awk '{print $1","$2","$3}' | sort -t, -k2 | sed -e "s|.\\$|.#|g" >> $SCRIPTS_LIST_FILE


cd $EXEC_PATH

echo "" > tmp.txt
while read line
do
	test=`is_object $line`
	name=`echo $line |cut -d, -f2 |sed -e "s/\\./,/g"`
	case "$test" in
		"object" )
			str=`echo $line |cut -d, -f2`
			array=`is_array $str`
			str="object,$array,root,$name"
			;;
		"instance" )
			str="instance,,root,$name"
			;;
		"parameter" )
			type=`get_param_type "$line"`
			str="parameter,$type,root,$name"
			;;
	esac
	echo "$str" >> tmp.txt
done <$SCRIPTS_LIST_FILE

#Remove instances from lines
cont=1
while [ "$cont" != "" ]; do
	sed -ri 's/,[0-9]+//' tmp.txt
	cont=`grep ",[0-9]+" tmp.txt`
done
cont=1
while [ "$cont" != "" ]; do
	sed -ri 's/,(#)[^,]+//' tmp.txt
	cont=`grep ",#" tmp.txt`
done
	
#Remove duplicated lines
awk '!a[$0]++' tmp.txt > tmp2.txt
mv tmp2.txt tmp.txt


gen_data_model_xml_file > inteno.xml

rm -rf tmp.txt
rm -rf $SCRIPTS_LIST_FILE

echo "Number of objects is $cnt_obj"
echo "Number of parameters is $cnt_param"

echo "End Of Generation"
