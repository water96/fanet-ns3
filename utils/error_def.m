function [ y ] = error_def( d )
%UNTITLED7 Summary of this function goes here
%   Detailed explanation goes here

    start = 5;
    N = length(d);
    y = [ (start:N)', zeros(N - start + 1, 4)];

    for i = start : N    
        [h, e] = histcounts(d(1:i));
        a = (e(1 : end - 1) + e(2 : end)) ./ 2;
        m = mean(d(1:i));
        s = sqrt(sum((d(1:i) - m).^2) / (i - 1));
        mym = sum(a .* h) / i;
        s_my = sqrt(sum(h .* (a - mym).^2) / (i - 1));
       
        y(i - start + 1, 2) = m;
        y(i - start + 1, 3) = s;
        y(i - start + 1, 4) = mym;
        y(i - start + 1, 5) = s_my;
    end
    
    %for i = start : N
    %    m = mean(d(1:i));
    %    s_shift = sqrt(sum((d(1:i) - m).^2) / (i - 1));
    %    y(i - start + 1, 2) = s_shift;
    %end

end

