# Project reason
I started this project to recreate the basic  Karel  IDE in C++, although it already exists, I still learned a lot from programming this.

# Update
Added in-line function and conditional naming;

# Instructions:  
 - 00000000 to move   
 - 00000001 to turn                     --include 4 digit base-19 input between paranthesis with 2 digit header and footer
 - 00000010 to set speed                --include 8 digit binary input between paranthesis
 - 00000011 to display current status
 - 00000100 to get ball
 - 00000101 to place ball
 - 00000110 to auto get ball
 - 00001001 to open paranthesis
 - 00001010 to close paranthesis
 - 10000111 to start function
 - 10001000 to start conditional
 - 10001001 to start else               --after a conditional, no name required


Turning is a 4 digit nonadecimal or base-19 code in between 4 zeroes:
 - Example: 004e4e00
 - First and last 2 digits are ignored for evaluation as follows: 4e4e 
 - The first 2 digits are turning in the xz plane
 - The second 2 digits are for turning in the xy plane (10) A B C D E F G H I (18)
 - 90 degrees is 4E, 180 is 99, 270 is E4, 360 is II   (max 19^2-1 or 361-1)

# Functions

 1. Use _10000111_ to create a new command
 2. Enter the name for the command.
 3. Enter content in between _00001001_ and _00001010_ 
 4. _Reminder: don't call the command you created in the command_

# Conditionals 
 1. For conditionals, use _10001000_
 2. Enter the name for the command 
 3. In between _00001001_ and _00001010_, write the condition first followed by the commands to execute if true
 4. Add _10001001_ with commands to execute if the initial condition is false in between _00001001_ and _00001010_.

For a condition use 8 digit binary inputs for the index, comparison operator, and the value to compare against.
The comparison operators are as the following:
-  "<" is 00000000, ">" is 00000001, "==" is 00000010, "!=" is 00000011
-  or 0 for '<', 1 for '>', 2 for '==', 3 for '!='
