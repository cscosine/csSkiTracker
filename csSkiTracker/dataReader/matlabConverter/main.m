close all
clear all
frameN = 1;

dir = '../../datasets/frame_data_bolshunov';

outfile = [dir '.std']; % ski tracker data
fo = fopen(outfile, 'w');
fprintf(fo, '#SKI_TRACKER_DATA_INPUT v1.0\n');

fprintf(fo, '#VIEW1_IMG_SIZE 720 576\n');
fprintf(fo, '#VIEW2_IMG_SIZE 720 576\n');

worldPoints = cumulateAllWorldPoints(dir);
nwp = size(worldPoints,2);
fprintf(fo, '#WORLD_POINTS %d\n', nwp);
for i = 1 : nwp
  fprintf(fo, '  %.6f %.6f %.6f\n', worldPoints(1,i), worldPoints(2,i), worldPoints(3,i));
end

readOk = true;
while(readOk)
  filename = [dir '/' sprintf('frame_%03d.mat', frameN)];
  c = exist(filename, 'file');
  if(c == 2)
    readOk = true;

    fd = load(filename);
    fd = fd.frame_data;

    % print view 1 only first time
    if(frameN == 1)
      s = viewData2String(fd.view1, '#CALIB_PTS_VIEW_1', '  ', worldPoints);
      fprintf(fo, s);
    end

    s= frameData2string(fd, '  ', worldPoints);
    fprintf(fo, '#FRAME %d\n', frameN);
    fprintf(fo, s);

    frameN = frameN + 1
  else
    readOk = false;
  end
end

fprintf('DONE! read %d frames\n', frameN);

fclose(fo);
