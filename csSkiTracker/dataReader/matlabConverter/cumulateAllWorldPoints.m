function wP = cumulateAllWorldPoints(dir)
  wP = zeros(3,0);
  frameN = 1;
  readOk = true;
  while(readOk)
    filename = [dir '/' sprintf('frame_%03d.mat', frameN)];
    c = exist(filename, 'file');
    if(c == 2)
      readOk = true;

      fd = load(filename);
      fd = fd.frame_data;

      for v = 1 : 2
        if(v == 1)
          worldPoints = fd.view1.world_pts(1:3,:);
        else
          worldPoints = fd.view2.world_pts(1:3,:);
        end


        for i = 1 : size(worldPoints,2)
          distV = sum((wP - worldPoints(:,i)).^2);
          idx = find(distV < eps);
          if(length(idx) == 0)
            % add
            wP = [wP worldPoints(:,i)];
          elseif(length(idx) == 1)
            % ok, replicate
          else
            assert(false)
          end
        end
      end
      frameN = frameN + 1
    else
    readOk = false;
    end

  end
endfunction
