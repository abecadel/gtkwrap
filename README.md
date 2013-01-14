gtkwrap
=======

GTK gui in bash.

Create Your gui in Glade(GtkBuilder) and use it in your shell scripts.

EXAMPLE 1:

1) create Your GUI in Glade, name signal handler You're going to use

2) create Your bash script, name functions the same as signal handlers in Your Glade project

      #!/bin/bash
      
      on_button1_clicked(){
        #do something on button clicked signal
      }
      
      ./gtk_wrap -f a.glade | while read line
      do
        $line
      done
      
EXAMPLE 2:
Simple calculator

1) create Glade project with 3 textview widgets and one button

2) name button clicked signal as on_button1_clicked

     #!/bin/bash
     
     on_button1_clicked(){
        echo "textview1 get_textview_text" > inpipe
        echo "textview2 get_textview_text" > inpipe
        read a < outpipe
        read b < outpipe
        let "c = $a + $b"
        echo "textview3 set_textview_text $c" > inpipe
     }

     ./gtk_wrap -f b.glade -i inpipe -o outpipe | while read func
     do
        $func
     done
   
