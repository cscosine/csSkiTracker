function s = viewData2String(v, header, prefix, worldPoints)
  nPoints = size(v.img_pts,2);

  s = [header sprintf(' %d\n', nPoints)];
  s = [s prefix sprintf('#PROJ_MATRIX\n')];
  for i = 1 : 3
    for j = 1 : 4
      s = [s prefix sprintf('  %.16f', v.P(i,j))];
    end
    s = [s sprintf('\n')];
  end


  assert(size(v.img_pts,1) == 3);
  assert(all(v.img_pts(3,:) == 1));

  assert(size(v.world_pts,1) == 4);
  assert(all(v.world_pts(4,:) == 1))

  assert(size(v.img_pts,2) == size(v.world_pts,2));

  s = [s prefix sprintf('#IMG_POINTS\n')];
  for i = 1 : nPoints
    s = [s prefix sprintf('  %.6f %.6f\n', v.img_pts(1,i), v.img_pts(2,i))];
  endfor

  s = [s prefix sprintf('#WORLD_POINTS_ID\n')];
  s = [s prefix '  '];
  for i = 1 : nPoints
    wP = v.world_pts(1:3,i);
    distV = sum((wP - worldPoints).^2);
    idx = find(distV < eps);
    assert(size(idx) == 1);
    s = [s sprintf('%d ', idx - 1)];
  endfor
  s = [s sprintf('\n')];

endfunction
