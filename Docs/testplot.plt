reset
set datafile separator ";"

#set style data histogram
#set style histogram cluster gap 1
#set style fill solid border -1
#set boxwidth 0.8

set xlabel "Megabytes"
set ylabel "Micro seconds"
#unset xtic
set grid ytics lt 0 lw 1
#set grid xtics lt 0 lw 1
set border 2
#set logscale y 2 
set yrange[0:100]
set xrange[:1050]

set terminal pdfcairo enh size 5, 5 font 'Times New Roman'
#set terminal window
 	
set style fill transparent solid 0.1 noborder
set key below right vertical

set output "poolLinear.pdf" 
set title "Pool Allocator Linear"
plot "../GEN/MemoryTest/poolAllocatorLinear.csv" using 1:3 title 'Malloc' w lines lw 10 lc rgb "#008800",\
"../GEN/MemoryTest/poolAllocatorLinear.csv" using 1:3 notitle w filledcurves y1=0 lw 3 lc rgb "#008800",\
"../GEN/MemoryTest/poolAllocatorLinear.csv" using 1:2 title 'Pool Allocator' w lines lw 10 lc rgb "#ff0000",\
"../GEN/MemoryTest/poolAllocatorLinear.csv" using 1:2 notitle w filledcurves y1=0 lw 3 lc rgb "#ff0000",\
"../GEN/MemoryTest/poolAllocatorLinear.csv" using 1:4 title 'Pool Allocator Single Thread' w lines lw 10 lc rgb "#0000ff",\
"../GEN/MemoryTest/poolAllocatorLinear.csv" using 1:4 notitle w filledcurves y1=0 lw 3 lc rgb "#0000ff"

set output "poolRandom.pdf"
set title "Pool Allocator Random"
plot "../GEN/MemoryTest/poolAllocatorRandom.csv" using 1:3 title 'Malloc' w lines lw 10 lc rgb "#008800",\
"../GEN/MemoryTest/poolAllocatorRandom.csv" using 1:3 notitle w filledcurves y1=0 lw 3 lc rgb "#008800",\
"../GEN/MemoryTest/poolAllocatorRandom.csv" using 1:2 title 'Pool Allocator' w lines lw 10 lc rgb "#ff0000",\
"../GEN/MemoryTest/poolAllocatorRandom.csv" using 1:2 notitle w filledcurves y1=0 lw 3 lc rgb "#ff0000",\
"../GEN/MemoryTest/poolAllocatorRandom.csv" using 1:4 title 'Pool Allocator Single Thread' w lines lw 10 lc rgb "#0000ff",\
"../GEN/MemoryTest/poolAllocatorRandom.csv" using 1:4 notitle w filledcurves y1=0 lw 3 lc rgb "#0000ff"

set output "stackSimple.pdf"
set yrange[0:0.09]
set xlabel "Number of 1000 objects"
set title "Stack Allocator Simple"
plot "../GEN/MemoryTest/stackAllocatorSimple.csv" using ($1/1000):3 title 'Malloc' w lines lw 10 lc rgb "#008800",\
"../GEN/MemoryTest/stackAllocatorSimple.csv" using ($1/1000):3 notitle w filledcurves y1=0 lw 3 lc rgb "#008800",\
"../GEN/MemoryTest/stackAllocatorSimple.csv" using ($1/1000):2 title 'Stack Allocator' w lines lw 10 lc rgb "#ff0000",\
"../GEN/MemoryTest/stackAllocatorSimple.csv" using ($1/1000):2 notitle w filledcurves y1=0 lw 3 lc rgb "#ff0000",\
"../GEN/MemoryTest/stackAllocatorSimple.csv" using ($1/1000):4 title 'Stack Allocator Single Thread' w lines lw 10 lc rgb "#0000ff",\
"../GEN/MemoryTest/stackAllocatorSimple.csv" using ($1/1000):4 notitle w filledcurves y1=0 lw 3 lc rgb "#0000ff"

set output "stackRandom.pdf"
set title "Stack Allocator Random"
plot "../GEN/MemoryTest/stackAllocatorRand.csv" using ($1/1000):3 title 'Malloc' w lines lw 10 lc rgb "#008800",\
"../GEN/MemoryTest/stackAllocatorRand.csv" using ($1/1000):3 notitle w filledcurves y1=0 lw 3 lc rgb "#008800",\
"../GEN/MemoryTest/stackAllocatorRand.csv" using ($1/1000):2 title 'Stack Allocator' w lines lw 10 lc rgb "#ff0000",\
"../GEN/MemoryTest/stackAllocatorRand.csv" using ($1/1000):2 notitle w filledcurves y1=0 lw 3 lc rgb "#ff0000",\
"../GEN/MemoryTest/stackAllocatorRand.csv" using ($1/1000):4 title 'Stack Allocator Single Thread' w lines lw 10 lc rgb "#0000ff",\
"../GEN/MemoryTest/stackAllocatorRand.csv" using ($1/1000):4 notitle w filledcurves y1=0 lw 3 lc rgb "#0000ff"

unset output
unset terminal

reset