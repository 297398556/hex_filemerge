This is a small program for merging the HEX address.
The merge operation performs as below:

  ->  If the HEX record type is 04 and the ELA high address is 0x8000 segment,
  
  ->  then merge the 0x8000 high addresses  with 0xA000 high address.
  
  ->  Sort the data records by low address, from the lowest to the highest.


Only supports single HEX file input, and the output is another HEX file.

这是一个HEX文件合并程序，将单个文件中的线性扩展地址段从0x8000段合并至0xA000段，
段内的低地址数据记录合并，并遵循升序排列。

仅支持单个文件的输入和输出。
