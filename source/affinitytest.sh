#!/bin/bash

# Take each IRQ file
for filename in /proc/irq/*; do
	# Output the IRQ files current affinity (will output the bitmask as a number)
	echo $filename/smp_affinity ":" $(cat $filename/smp_affinity)
done