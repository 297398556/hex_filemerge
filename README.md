This is a small program for merging the HEX address.
The merge operation performs as below:

  ->  If the HEX record type is 04 and the ELA high address is 0x8000 segment,
  
  ->  then merge the 0x8000 high addresses  with 0xA000 high address.
  
  ->  Sort the data records by low address, from the lowest to the highest.


Only supports single HEX file input, and the output is another HEX file.
