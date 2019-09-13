#!/bin/bash

if [ $# != 2 -o "$1" == "-help" ]; then
    echo "usa: chatty.conf tempo"
    exit 1
fi

filename=$1
tempo=$2

if [ ! -f $filename ]; then
    echo "il file $filename non esiste o non e' un file regolare"
    exit 1
fi

if [ ! -s $filename ]; then
    echo "il file $filename e' vuoto"
    exit 1
fi

exec 3<$filename   # apro il file il lettura

#leggo ogni linea del file
while read -u 3 linea; do
	read -r -a elem <<< "$linea"
	if [ "${elem[0]}" == "DirName" ]; then
		stringa=${elem[2]}
	fi
done

cd $stringa
#se ho indicato come tempo 0 stampo in output i file presenti all'interno della cartella
if [ $tempo == 0 ]; then
    ls
    exit 1
fi

#archivio i file  
tar -zcvf chatty.tar.gz $(find $stringa -mmin +$tempo)

for f in $stringa
    do
       rm -i -R $f
done

