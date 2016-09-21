function [ on ] = rl_channel_on( channels, number )

mask = 2^number;
if bitand(channels, mask) > 0
    on = 1;
else
    on = 0;
end

end

