# @title Mercator
# @tags distorts

filter mercator (image in)
    in(xy*xy:[cos(pi/2*y+t*2*pi),1])
end
