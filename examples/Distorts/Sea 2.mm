filter sea2 (image in, float amp: 0-100 (10), float wv: 1-100 (10))
    in(xy+xy:[0,amp*sin(t*2*pi+wv*Y*(-y+Y+60)^-1)])
end