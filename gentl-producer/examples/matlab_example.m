%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Copyright (c) 2019 Nerian Vision GmbH
%
% Permission is hereby granted, free of charge, to any person obtaining a copy
% of this software and associated documentation files (the "Software"), to deal
% in the Software without restriction, including without limitation the rights
% to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
% copies of the Software, and to permit persons to whom the Software is
% furnished to do so, subject to the following conditions:
%
% The above copyright notice and this permission notice shall be included in
% all copies or substantial portions of the Software.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function matlab_example()

% The Q matrix for the current camera calibration. This needs to
% be changed when the cameras are recalibrated! You can find the
% current Q matrix on the review calibration page of the device's
% web interface.
Q = [1. 0. 0. -300;
    0. 1. 0. -200;
    0. 0. 0.  750;
    0. 0. 6.5 0.];

% First reset the image acquisition
imaqreset

% Search the disparity device
deviceID = -1;
hwinfo = imaqhwinfo('gentl');
for dev = hwinfo.DeviceInfo
    if ~isempty(strfind(dev.DeviceName, "disparity"))
        deviceID = dev.DeviceID;
    end
end

% Test if the disparity device was found
if deviceID == -1
    error('Could not find disparity device');
end

% Open device
vid = videoinput('gentl', deviceID);

% Configure video input to capture 1000 frames
vid.TriggerRepeat = 1000;
vid.FramesPerTrigger = 1;

% Start acquiring frames.
start(vid);

while(isrunning(vid))
    % Acquire a new disparity map
    disparityMap = getdata(vid,1);

    % Get arrays of x and y coordinates
    [x, y] = meshgrid(1:size(disparityMap, 2), 1:size(disparityMap, 1));

    % Create matrix of 2D homogeneous coordinates
    numpoints = numel(disparityMap);
    points2dHomog = [
        reshape(x, 1, numpoints);
        reshape(y, 1, numpoints);
        reshape(double(disparityMap)/16, 1, numpoints);
        ones(1, numpoints)];

    % Project 2D coordinates to 3D
    points3dHomog = Q * points2dHomog;

    % Convert homogeneous coordinates to 3D point map
    points3d = bsxfun(@times, points3dHomog(1:3, :), 1./points3dHomog(4, :));

    % Extract x, y and z channel
    X = reshape(points3d(1, :), size(disparityMap));
    Y = reshape(points3d(2, :), size(disparityMap));
    Z = reshape(points3d(3, :), size(disparityMap));

    % Set all points with invalid disparities or depth larger than 10 m to invalid
    maxDist = 10;
    invalid = (disparityMap == 4095) | (Z > maxDist);
    X(invalid) = NaN;
    Y(invalid) = NaN;
    Z(invalid) = NaN;

    % Display depth map
    figure(1);
    imagesc(Z, [0, maxDist]);

    % Display 3D point cloud (rotated)
    figure(2);
    pcshow(cat(3, X, Z, -Y));

    drawnow;
end

% Clean up
delete(vid);
clear vid;
