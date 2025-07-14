function s = frameData2string(fd, prefix, worldPoints)
  s = '';
  #####################
  nMatches = size(fd.matchingPts.view1,1);
  s = [s prefix sprintf('#MATCHING_PTS %d\n', nMatches)];
  assert(size(fd.matchingPts.view1,1) == size(fd.matchingPts.view2,1));
  assert(size(fd.matchingPts.view1,1) == size(fd.matchingPts.world,1));
  s = [s prefix sprintf('  #VIEW_1\n')];
  for i = 1 : nMatches
    s = [s prefix sprintf('    %.6f %.6f\n', fd.matchingPts.view1(i,1), fd.matchingPts.view1(i,2))];
  endfor
  s = [s prefix sprintf('  #VIEW_2\n')];
  for i = 1 : nMatches
    s = [s prefix sprintf('    %.6f %.6f\n', fd.matchingPts.view2(i,1), fd.matchingPts.view2(i,2))];
  endfor
  s = [s prefix sprintf('  #WORLD\n')];
  for i = 1 : nMatches
    s = [s prefix sprintf('    %.6f %.6f %.6f\n', fd.matchingPts.world(i,1), fd.matchingPts.world(i,2), fd.matchingPts.world(i,3))];
  endfor

  #####################
  nSkier = size(fd.view1.skier_pts,2);
  s = [s prefix sprintf('#SKIER_PTS %d\n', nSkier)];
  assert(size(fd.view1.skier_pts,2) == size(fd.view2.skier_pts,2));
  s = [s prefix sprintf('  #VIEW_1\n')];
  for i = 1 : nSkier
    s = [s prefix sprintf('    %.6f %.6f\n', fd.view1.skier_pts(1,i), fd.view1.skier_pts(2,i))];
  endfor
  s = [s prefix sprintf('  #VIEW_2\n')];
  for i = 1 : nSkier
    s = [s prefix sprintf('    %.6f %.6f\n', fd.view2.skier_pts(1,i), fd.view2.skier_pts(2,i))];
  endfor


  #####################
  # skip, write only once in header
  ## s = [s viewData2String(fd.view1, [prefix '#CALIB_PTS_VIEW_1'], [prefix '  '], worldPoints)];
  #####################
  s = [s viewData2String(fd.view2, [prefix '#CALIB_PTS_VIEW_2'], [prefix '  '], worldPoints)]; 
endfunction
