function [ oup ] = rl_get_rate( inp )

if inp == 0
    oup = 64000;
elseif inp == 256
    oup = 32000;
elseif inp == 512
    oup = 16000;
elseif inp == 768
    oup = 8000;
elseif inp == 1024
    oup = 4000;
elseif inp == 1280
    oup = 2000;
elseif inp == 1536
    oup = 1000;
end

end

