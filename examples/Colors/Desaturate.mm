# @title Desaturate
# @tags colors

filter desaturate (image in)
    p=in(xy);
    grayaColor(gray(p),alpha(p))
end
