#!/bin/bash

GLADE_FILE="demo2.glade"

IN="/tmp/${0}.${$}.in"
OUT="/tmp/${0}.${$}.out"

GTK_WRAP="../gtk-wrap -v -f $GLADE_FILE -i $IN -o $OUT"


on_button1_clicked(){
 
    echo "textview1 get_textview_text a"
    echo "textview2 get_textview_text a"
    read a < $OUT
    read b < $OUT
    let "c = $a + $b"
    echo "textview3 set_textview_text $c"

} > $IN


$GTK_WRAP | while read func
do
    $func
done
