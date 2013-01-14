#!/bin/bash

GLADE_FILE="demo1.glade"
GTK_WRAP="../gtk-wrap -f $GLADE_FILE"

on_button1_clicked(){
    echo button1 clicked
}

on_togglebutton1_toggled(){
    echo toggled
}

on_window1_destroy(){
    echo window destroyed
}

$GTK_WRAP | while read line
do
    $line
done
