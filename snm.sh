interface_name="br0"
communities_num=10
edges_num=10

machine_ip=$(ifconfig $interface_name | grep "inet addr:" | cut -d ":" -f 2 | cut -d " " -f 1)
machine_id=$(echo "$machine_ip" | cut -d "." -f 4)

#echo $machine_id

stop_edges()
{
	local i=0
	while [ $i -lt $edges_num ]; do
		echo "stop" | nc -w 1 -u 127.0.0.1 5644
		(( i++ ))
	done
}



#./edge -v -v -f -d edge2 -a 5.8.4.3 -c commmmmm

start_edges()
{
	local mod=$(expr $edges_num % $communities_num)
	if [ $mod -ne 0 ]; then
		echo "not equally distributed"
		return
	fi
	
	local edges_per_comm=$(expr $edges_num / $communities_num)

	for ci in `seq -w 1 $communities_num`; do
		local epci=0
		echo $ci

		for epci in `seq 1 $edges_per_comm`; do

			local comm_id=$(echo $ci | sed 's/^0//')

			# edge ip: 5.machine_id.comm_id.edge_id
			./edge -v -v -f -d edge$ci -a 5.$machine_id.$comm_id.$epci -c comm$ci -l 141.85.224.2$ci:120$ci > EDGE_LOG_comm$ci &

			(( epci++ ))
		done
	done
}

if [ "$1" == "start" ]; then
	start_edges
fi
if [ "$1" == "stop" ]; then
	stop_edges
fi
if [ "$1" == "supernode" ]; then
	if [ $machine_id -eq 203 ]; then
		./supernode -v -v -f -l 11$machine_id -s 12$machine_id > SN_LOG_$machine_id &
	else	
		if [ ! -f SN_SNM_* ]; then
			# first run
			./supernode -v -v -f -l 11$machine_id -s 12$machine_id -i 141.85.224.203:12203 > SN_LOG_$machine_id &
		else
			./supernode -v -v -f -l 11$machine_id -s 12$machine_id > SN_LOG_$machine_id &
		fi
	fi
fi

