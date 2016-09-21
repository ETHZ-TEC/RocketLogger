function [ b ] = count_bits( a )

size_bits = 32;
mask = 1;
b = 0;

for i=1:size_bits
    if bitand(a,mask) > 0
        b = b + 1;
    end
    mask = mask * 2;
end

end

