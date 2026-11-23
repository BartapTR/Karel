# Project reason
I started this project to recreate the basic  Karel  IDE in C++, although it already exists, I still learned a lot from programming this.

# Instructions:  
 - 000 to move   
 - 001 to turn          --include 4 digit base-19 input between any 2 characters
 - 010 to set speed     --include 4 digit binary input between any 2 characters
 - 011 to display current status
 - 100 to get ball
 - 101 to place ball
 - 110 to auto get ball

Turning is a 4 digit nonadecimal or base-19 code: 
 - The first 2 digits are turning in the xz plane
 - The second 2 digits are for turning in the xy plane A(10) B C D E F G H I(18)
 - First 2 digits for turning are for the xz direction, the next two are for xy direction:
 - 90 degrees is 4E, 180 is 99, 270 is E4, 360 is II   (max 19^2-1 or 361-1)

# Functions and conditionals

Use _fun_ to create a command then enter the name for the command.
_Reminder: don t call the command you created in the command_

For conditionals, use _111_ to start and _con_ to end the conditional, 
then use 3 digit binary inputs for the index, comparison operator, and value to compare against

The comparison operators are as the following:
"<" is 000, ">" is 001, "==" is 010, "!=" is 011

NEVER USE _111_ unless for opening conditionals as you need to have equal numbers of _111_ and _con_
    