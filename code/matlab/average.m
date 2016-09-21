function [ sum  ] = average( values, index, buffer_size )

sum=0;
for i=1:buffer_size
    sum = sum + values(i,index);
end
sum = sum / buffer_size;

end

