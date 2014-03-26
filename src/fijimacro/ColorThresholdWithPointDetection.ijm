// ColorThresholdWithPointDetection.ijm
// Rich Stoner
//
// Takes a very large image, performs and RGB thresholds it, does a point detection, and saves the result to a csv
//

filename = getArgument();
filecomponents = split(filename,".");

filebase = filecomponents[0];
if(lengthOf(filecomponents) > 2)
{
	for(i= 1; i<lengthOf(filecomponents)-1; i++)
	{
		filebase += '.' + filecomponents[i];
	}
}

open(filename);

min=newArray(3);
max=newArray(3);
filter=newArray(3);
a=getTitle();

print("Performing RGB filter...");

run("HSB Stack");
run("Convert Stack to Images");
selectWindow("Hue");
rename("0");
selectWindow("Saturation");
rename("1");
selectWindow("Brightness");
rename("2");

min[0]=21;
max[0]=47;
filter[0]="stop";
min[1]=0;
max[1]=255;
filter[1]="pass";
min[2]=0;
max[2]=200;
filter[2]="pass";

for (i=0;i<3;i++){
  selectWindow(""+i);
  setThreshold(min[i], max[i]);
  run("Convert to Mask");
  if (filter[i]=="stop")  run("Invert");
}

imageCalculator("AND create", "0","1");
imageCalculator("AND create", "Result of 0","2");
for (i=0;i<3;i++){
  selectWindow(""+i);
  close();
}

//saveAs("tif", filebase + "_filtstrong.tif");

print("Image calculation...");

selectWindow("Result of 0");
close();
selectWindow("Result of Result of 0");
rename("mask");

open(filename);

rename("original");
imageCalculator("OR create", "original", "mask");
selectWindow("original");
close();

//selectWindow("mask");
//print("Saving: " + filebase + "_mask.jpg");
//saveAs("tif", filebase +"_mask.tif");

selectWindow("Result of original");

// print("Saving: " + filebase + "_filtstrong.jpg");
//saveAs("tif", filebase + "_filtstrong.tif");

run("8-bit"); // make 8-bit
setAutoThreshold();

run("Convert to Mask");
run("Open");

// run("Set Scale...", "distance=1 known=2 pixel=1.000 unit=um");

run("Set Measurements...", "center bounding area redirect=None decimal=3");

run("Analyze Particles...", "size=0.00-400.0 circularity=0.00-1.00 display clear add");


csv_filename = filebase + "_points.csv";



print(csv_filename);

saveAs("Results", csv_filename);

close();
close();
exit();
