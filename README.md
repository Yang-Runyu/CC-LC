# CC-LC
CC-LC is a lossless codec for feature compression. We also provide an end-to-end image compression method using CC-LC as the entropy coding method. The platform is VS2017.

  There are 9 kinds of commands.  
  -h(help information)  
  -i(input file which only support .yuv now)  
  -o(output file)  
  -r(row of the feature map)  
  -c(col of the feature map)  
  -f(number of channels)  
  -s(skip number of channels)  
  for example:  
  encoder.exe -i D:/input.yuv -r 64 -c 96 -f 128 -o output.bin  
  decoder.exe -i output.bin -o recover.yuv  

