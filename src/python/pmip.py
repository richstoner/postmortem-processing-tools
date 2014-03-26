# -*- coding: utf-8 -*-
import os
import skimage
import numpy as np
import scipy as sp
from scipy import ndimage
from skimage import color, filter

from skimage import measure
from scipy import signal
import glob
from skimage.transform import pyramids
import workerpool


class Processing(object):
    """docstring for Processing"""

    def __init__(self, _specimen):
        #self.specimen = _specimen.replace('.', '_')

        self.s = _specimen  # sending entire Specimen object

        self.specimen = self.s.subjectName.replace('.', '_')

        self.basedir = '/data/reconstruction'
        self.scriptBaseDir = '/vagrant/src/fijimacro'

        self.dirs = {}
        self.dirs['spec'] =  os.path.join(self.basedir, 'specimens', self.specimen)

        self.dirs['regraw'] =       os.path.join(self.dirs['spec'], 'register_raw')

        self.dirs['regcontrast'] =  os.path.join(self.dirs['spec'], 'register_contrast')

        self.dirs['regsource'] = os.path.join(self.dirs['spec'], 'register_source')

        self.dirs['regtarget'] = os.path.join(self.dirs['spec'], 'register_target')

        self.dirs['video'] = os.path.join(self.dirs['spec'], 'video')
        
        self.dirs['detect'] = os.path.join(self.dirs['spec'], 'detect_raw')

        self.dirs['points'] = os.path.join(self.dirs['spec'], 'detect_points')

        self.dirs['regpoints'] = os.path.join(self.dirs['spec'], 'register_points')

        self.dirs['regdensity'] = os.path.join(self.dirs['spec'], 'register_density')        

        self.dirs['regstack'] = os.path.join(self.dirs['spec'], 'register_stack')

        self.processing_status = {}

    def _validateEnvironment(self):

        dir_list = []
        dir_list.append('/data/reconstruction')
        dir_list.append('/data/reconstruction/specimens')

        bReturnVal = True

        for d in dir_list:
            if not os.path.exists(d):
                print 'missing : %s' % d
                bReturnVal = False
            else:
                print 'found   : %s' % d

        return bReturnVal


    def _buildDirectoryStructure(self):

        for dd in self.dirs.keys():
            if not os.path.exists(self.dirs[dd]):
                os.makedirs(self.dirs[dd])

        print 'directories for %s created' % self.specimen


    def initEnv(self): 

        self._printTitle('initEnv')

        if not self._validateEnvironment():
            print 'environment not complete, please check configuration'
            return

        self._buildDirectoryStructure()


    #pe.generateFramesForImageList(e.getSortedImageList())

    def collectImagesForRegistration(self):
        self._printTitle('collectRaw, downsample by 2^4')

        self.processing_status['regraw'] = self.collectRawGenerics(4, self.dirs['regraw'])



    def collectImagesForCellDetection(self):
        self._printTitle('collectRaw, downsample by 2^1')
        self.processing_status['detect'] = self.collectRawGenerics(1, self.dirs['detect'])



    def collectImagesForGeneration(self):

        import glob

        dscImageList = glob.glob(os.path.join(self.dirs['points'], '*.area'))
        dscImageList.sort()

        self.processing_status['regpointa'] = dscImageList      

        dscImageList = glob.glob(os.path.join(self.dirs['points'], '*.centroid'))
        dscImageList.sort()

        self.processing_status['regpointc'] = dscImageList      


        dscImageList = glob.glob(os.path.join(self.dirs['regpoints'], '*.reg'))
        dscImageList.sort()

        self.processing_status['regpoints'] = dscImageList    


        dscImageList = glob.glob(os.path.join(self.dirs['regsource'], '*'))
        dscImageList.sort()

        self.processing_status['regsource'] = dscImageList


        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*.jpg'))
        dscImageList.sort()

        self.processing_status['regtarget'] = dscImageList

        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*txt'))
        dscImageList.sort()

        self.processing_status['regxform'] = dscImageList      

        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*mox'))
        dscImageList.sort()

        self.processing_status['regmox'] = dscImageList      

        dscImageList = glob.glob(os.path.join(self.dirs['regcontrast'], '*-c.jpg'))
        dscImageList.sort()

        self.processing_status['regcontrast'] = dscImageList







    def collectRawGenerics(self, DOWNSAMPLE, _dir):

        if self.s.remoteSpecimen == True:
        
            print '-> collecting images from remote source'
            # download downsampled images

            import aibs
            reload(aibs)
            api = aibs.api()
            val =  api.getDSImagesFromListToPath(self.s.getSortedImageList(), _dir, downsample=DOWNSAMPLE)

            return val




        else:
            
            # print '-> collecting images from local source'
            return self.getDSImagesFromLocalToPath(self.s.getSortedImageList(), _dir, downsample=DOWNSAMPLE)
        


    def getDSImagesFromLocalToPath(self, imageList, path, downsample=5):

        list_created = []

        for img in imageList:
            list_created.append(img.generateDownSampleConversion(path, ds=downsample))

        return list_created

            


    def createContrast(self):

        import glob

        list_created = []

        self._printTitle('createContrast')

        sourceList = self.processing_status['regraw']
        targetList = glob.glob(os.path.join(self.dirs['regcontrast'], '*-c.jpg'))

        dsImageList = []

        for s in sourceList:

            bFound = False

            for t in targetList:
                if os.path.basename(s).split('.')[0] in os.path.basename(t):
                    bFound = True

            if not bFound:
                dsImageList.append(s)
            else:
                list_created.append(s)
        
        # generate contrast image
        # ISSUE: only generates contrast in reg-raw dir, requires additional step to copy, convert, and delete
        self._executeFIJIScript('REG-filter.ijm', dsImageList)
        
        dscImageList = glob.glob(os.path.join(self.dirs['regraw'], '*-c.jpg'))
        dscImageList.sort()



        for dsc in dscImageList:
            cmdstr = '/usr/bin/convert %s -resize 50%% %s' % (dsc, dsc.replace('register_raw', 'register_contrast'))
            pipe = os.popen(cmdstr)
            for e in pipe:
                pass

            cmdstr = 'rm %s' % (dsc)
            print cmdstr
            pipe = os.popen(cmdstr)
            for e in pipe:
                pass
            
            image_created = dsc.replace('register_raw', 'register_contrast')
            list_created.append(image_created)

        dscImageList = glob.glob(os.path.join(self.dirs['regcontrast'], '*-c.jpg'))
        dscImageList.sort()

        self.processing_status['regcontrast'] = dscImageList





    def createContrastUsingSK(self):

        self._printTitle('createContrast with scikit image')

        # get list of ds images
        import glob
        import os
        dsImageList = glob.glob(self.dirs['regraw'] + '/*-DSx4.jpg')
        dsImageList.sort()

        from scipy import ndimage
        import scipy.misc
        import numpy as np
        import skimage

        # from skimage import color, filter, exposure, transform

        for file_to_use in dsImageList:

            print file_to_use

            outputname = os.path.join(self.dirs['regcontrast'], os.path.basename(file_to_use)).replace('.jpg', '-csk.jpg')

            if not os.path.exists(outputname):

                image = ndimage.imread(file_to_use)
                image_gray = skimage.img_as_uint(skimage.color.rgb2gray(image))
                img_eq = skimage.exposure.equalize_hist(image_gray)
                elevation = skimage.filter.sobel(img_eq)
                elevation = ndimage.gaussian_filter(elevation, 5)

                img_to_write = np.zeros((3000,3000))
                y_offset = round((img_to_write.shape[0] - elevation.shape[0])/2)
                x_offset = round((img_to_write.shape[1] - elevation.shape[1])/2)

                img_to_write[y_offset:elevation.shape[0]+y_offset,x_offset:elevation.shape[1] + x_offset] = elevation

                outputname = os.path.join(self.dirs['regcontrast'], os.path.basename(file_to_use)).replace('.jpg', '-csk.jpg')

                img_to_write = skimage.transform.pyramids.pyramid_reduce(img_to_write)
                scipy.misc.imsave(outputname, img_to_write)




    def createFrames(self, userange=[]):

        self._printTitle('createFrames')

        list_created = []        

        import glob

        # dscImageList = glob.glob(os.path.join(self.dirs['regcontrast'], '*-c.jpg'))
        dscImageList = self.processing_status['regcontrast']
        dscImageList.sort()

        import shutil

        for n, dsc in enumerate(dscImageList):
            frameName = '%s/frame%04d.jpg' % (self.dirs['regsource'], n)
            if not os.path.exists(frameName):
                shutil.copy(dsc, frameName)

            list_created.append(frameName)


        self.processing_status['regsource'] = list_created





    def register(self, userange=[]):

        self._printTitle('register')

        import glob
        files_to_use = self.processing_status['regsource']

        first_file = '%s/frame0000.jpg' % (self.dirs['regsource'])

        first_reg_file = '%s/register0000.jpg' % (self.dirs['regtarget'])

        cmdstr ='cp -v %s %s' % (first_file, first_reg_file)
        pipe = os.popen(cmdstr, 'r')
            
        cmdstr = '/vagrant/bin/RigidBodyImageRegistration %s/frame%%04d.jpg ' \
                 '%s/register%%04d.jpg %d 0' % (self.dirs['regsource'], self.dirs['regtarget'], len(files_to_use))

        print cmdstr
        pipe = os.popen(cmdstr, 'r')
        for e in pipe:
            print e

        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*.jpg'))
        dscImageList.sort()

        self.processing_status['regtarget'] = dscImageList

        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*txt'))
        dscImageList.sort()

        self.processing_status['regxform'] = dscImageList      

        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*mox'))
        dscImageList.sort()

        self.processing_status['regmox'] = dscImageList      



    def collectRegisteredImages(self):

        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*.jpg'))
        dscImageList.sort()

        self.processing_status['regtarget'] = dscImageList

        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*txt'))
        dscImageList.sort()

        self.processing_status['regxform'] = dscImageList      

        dscImageList = glob.glob(os.path.join(self.dirs['regtarget'], '*register*mox'))
        dscImageList.sort()

        self.processing_status['regmox'] = dscImageList      






    def runDetection(self):



        self._printTitle('detectPoints')

        files_to_use = self.processing_status['detect']

        for f in files_to_use:
            print f
            f_a = os.path.join(self.dirs['points'], os.path.basename(f) + '.area')
            f_c = os.path.join(self.dirs['points'], os.path.basename(f) + '.centroid')            

            if not os.path.exists(f_a):

                im = ndimage.imread(f)
                imHSV = color.rgb2hsv(im)

                imsat = imHSV[:,:,1]
                satThreshold = np.zeros_like(imsat)
                satThreshold[imsat > 0.05] = 1

                fill_holes = ndimage.binary_fill_holes(satThreshold)
                remove_noise = ndimage.binary_opening(fill_holes, structure=np.ones((3,3))).astype(np.int)
                labeld_image, count = ndimage.label(remove_noise)
                regions = measure.regionprops(labeld_image, properties=['Area', 'Centroid'])

                a = []
                c = []

                for r in regions:
                    a.append(r['Area'])
                    c.append(r['Centroid'])


                np.savetxt(f_a, a)
                np.savetxt(f_c, c)


        dscImageList = glob.glob(os.path.join(self.dirs['points'], '*.area'))
        dscImageList.sort()

        self.processing_status['regpointa'] = dscImageList      

        dscImageList = glob.glob(os.path.join(self.dirs['points'], '*.centroid'))
        dscImageList.sort()

        self.processing_status['regpointc'] = dscImageList      



    class DetectionJob(workerpool.Job):



        "test job"
        def __init__(self, f, dir_to_use):
            self.file_ = f # The url we'll need to download when the job runs

        def run(self):

            # import skimage
            # import numpy as np
            # import scipy as sp
            # from scipy import ndimage
            # from skimage import color, filter

            # from skimage import measure
            # from scipy import signal
            # import glob
            # import os
            # from skimage.transform import pyramids

            print self.file_

            f_a = os.path.join(dir_to_use, os.path.basename(self.file_) + '.area')
            # f_c = os.path.join(dir_to_use, os.path.basename(self.file_) + '.centroid')            

            print f_a
            # if not os.path.exists(f_a):

                # print f_a

                # im = ndimage.imread(self.file_)
                # imHSV = color.rgb2hsv(im)

                # imsat = imHSV[:,:,1]
                # satThreshold = np.zeros_like(imsat)
                # satThreshold[imsat > 0.05] = 1

                # fill_holes = ndimage.binary_fill_holes(satThreshold)
                # remove_noise = ndimage.binary_opening(fill_holes, structure=np.ones((3,3))).astype(np.int)
                # labeld_image, count = ndimage.label(remove_noise)
                # regions = measure.regionprops(labeld_image, properties=['Area', 'Centroid'])

                # a = []
                # c = []

                # for r in regions:
                #     a.append(r['Area'])
                #     c.append(r['Centroid'])


                # np.savetxt(f_a, a)
                # np.savetxt(f_c, c)


    def runDetectionThreaded(self):


        self._printTitle('detectPoints')
        
        files_to_use = self.processing_status['detect']

        pool = workerpool.WorkerPool(size=2)

        for f in files_to_use:
            job = self.DetectionJob(f, self.dirs['points'])
            pool.put(job)

        pool.shutdown()
        pool.wait()

        import glob

        dscImageList = glob.glob(os.path.join(self.dirs['points'], '*.area'))
        dscImageList.sort()

        self.processing_status['regpointa'] = dscImageList      

        dscImageList = glob.glob(os.path.join(self.dirs['points'], '*.centroid'))
        dscImageList.sort()

        self.processing_status['regpointc'] = dscImageList      







    def savepointsForImage(img):
        print(img)
        im = ndimage.imread(img)
        imHSV = color.rgb2hsv(im)

        imsat = imHSV[:,:,1]
        satThreshold = np.zeros_like(imsat)
        satThreshold[imsat > 0.05] = 1

        fill_holes = ndimage.binary_fill_holes(satThreshold)
        remove_noise = ndimage.binary_opening(fill_holes, structure=np.ones((3,3))).astype(np.int)
        
        labeld_image, count = ndimage.label(remove_noise)
        
        regions = measure.regionprops(labeld_image, properties=['Area', 'Centroid'])

        point_list = []
        for reg in regions:
            c = reg['Centroid']
            point_to_convolve = (int(round(c[0])),int(round(c[1])))
        #    print point_to_convolve
            point_list.append(point_to_convolve)
        
        point_list_name = os.path.join(pe.dirs['stack'], os.path.basename(img).replace('jpg', 'txt'))
        np.savetxt(point_list_name, point_list)
        
        softimg = np.zeros_like(satThreshold)
        for i,n in enumerate(point_list):
            softimg[n[0],n[1]] = 1

        improc = ndimage.filters.gaussian_filter(softimg, 100, mode='constant')    
        

        fullsavename = os.path.join(pe.dirs['stack'], os.path.basename(img).replace('jpg', 'png'))
        ds4savename = os.path.join(pe.dirs['stack'], os.path.basename(img).replace('jpg', 'png').replace('DSx1', 'DSx4'))
        print fullsavename
        print ds4savename
        
        img_to_write = skimage.transform.pyramids.pyramid_reduce(improc, downscale=4)
        
        sp.misc.imsave(fullsavename, improc)
        sp.misc.imsave(ds4savename, img_to_write)        
































    def _printTitle(self, title):
        print ''
        titlestr = '* ' + title
        print titlestr
        print '-'*80


    def _executeFIJIScript(self, scriptName, fileInput, force=False):
        ''' takes the imagej script name and an array of file inputs'''
        import os

        fijiCommandString = '/home/vagrant/Fiji.app/fiji-linux64 -Xms10000m -batch %s %s'

        if not os.path.exists(os.path.join(self.scriptBaseDir,scriptName)):
            print('Script %s not found' % scriptName)
            return
        else:

            print('Executing %s on %d files' % (scriptName, len(fileInput)))
            
            for f_to_proc in fileInput:           

                expected_out = f_to_proc.split('.')[0] + '-c.jpg'
                if not os.path.exists(expected_out) or force:
                    commandToRun = fijiCommandString % (os.path.join(os.path.abspath(self.scriptBaseDir), scriptName), f_to_proc)
                    #print(commandToRun)
                    pipe = os.popen(commandToRun)
                    for e in pipe:
                        # print(e)
                        pass

                    print 'created: %s' % expected_out
                else:
                    print 'exists : %s' % expected_out
                    pass



    def _executeFijiExtract(self, scriptName, fileInput, force=False):
        ''' takes the imagej script name and an array of file inputs'''
        import os

        fijiCommandString = '/home/vagrant/Fiji.app/fiji-linux64 -Xms3000m --headless -batch %s %s'

        if not os.path.exists(os.path.join(self.scriptBaseDir,scriptName)):
            print('Script %s not found' % scriptName)
            return
        else:

            print('Executing %s on %d files' % (scriptName, len(fileInput)))
            
            for f_to_proc in fileInput:           

                expected_out = f_to_proc.split('.')[0] + '-c.jpg'
                if not os.path.exists(expected_out) or force:
                    commandToRun = fijiCommandString % (os.path.join(os.path.abspath(self.scriptBaseDir), scriptName), f_to_proc)
                    print(commandToRun)
                #     pipe = os.popen(commandToRun)
                #     for e in pipe:
                #         #print(e)
                #         pass

                #     print 'created: %s' % expected_out
                # else:
                #     print 'exists : %s' % expected_out
                #     pass

             




    

    def registerPoints(self):
            
        working_list =  self.s.getSortedImageList()

        action_list = []

        for n, valid_img in enumerate(working_list):
            point_source = '%s/%s-DSx1.txt' % (self.dirs['stack'],  valid_img.tag)
            
            if n > 0:
                xform = '%s/register%04d.jpg.0.txt' % (self.dirs['regtarget'], n )
                action_list.append([point_source, xform])
                
            else:
                xform = None
                action_list.append([point_source, xform])        

        #print action_list
        
        for a in action_list:
            for b in a:
                if type(b) != type(None):
                    if os.path.exists(b):
                        print 'found   : %s' % b
                    else:
                        print 'missing : %s' % b


    def extractPoints(self):
        self._printTitle('extractPoints')

        # get list of ds images
        import glob 
        dsImageList = glob.glob(os.path.join(self.dirs['detect'], '*.jpg'))
        dsImageList.sort()

        #for n, ds in enumerate(dsImageList):
        self._executeFijiExtract('ColorThresholdWithPointDetection.ijm', dsImageList)                    
        #self._executeFIJIScript('REG-filter-red50.jim', dsImageList)   





    

    def generateSummaryTable(self):
        import glob
        import os

        dscImageList = glob.glob(os.path.join(self.dirs['regraw'], '*4.jpg'))
        dscImageList.sort()

        htmlString = '<div>Original - Contrast - Contrast Scikit - Registered</div'

        for n,dsc in enumerate(dscImageList):
            htmlString += '<div>'
            basename = dsc.split('.')[0].replace('/vol/', 'files/')

            normal = '<img style="width: 150px; margin:3px;" src="//192.2.2.2/%s.jpg"/>' % basename
            contrast = '<img style="width: 150px; margin:3px;" src="//192.2.2.2/%s-c.jpg"/>' % (basename.replace('raw', 'contrast'))
            contrastSK = '<img style="width: 150px; margin:3px;" src="//192.2.2.2/%s-csk.jpg"/>' % (basename.replace('raw', 'contrast'))

            reg_url = os.path.dirname(basename).replace('raw','target') + '/register%04d.jpg' % n
            reg =   '<img style="width: 150px; margin:3px;" src="//192.2.2.2/%s"/>' % reg_url
            
            htmlString += '<h5>%s</h5>' % basename
            htmlString += normal
            htmlString += contrast
            htmlString += contrastSK
            htmlString += reg
            htmlString += "</div>"

        return htmlString


    def listSubjectDirectory(self):
        import glob, pprint
        dirlist = glob.glob(self.dirs['spec'] + '/**')
        for dl in dirlist:
            print '[%d files] %s' % (len(glob.glob(dl + '/*')), dl)
        


    def clearRawDirectory(self):
        ''' deletes all files downloaded to or copied to the raw directory '''
        import os
        os.popen('sudo rm -rvf %s/*' % self.dirs['raw'])

    def clearContrastDirectory(self):
        ''' deletes all files downloaded to or copied to the contrast directory '''
        import os
        os.popen('sudo rm -rvf %s/*' % self.dirs['contrast'])

    def clearRegisterSourceDirectory(self):
        ''' deletes all files downloaded to or copied to the regsource directory '''
        import os
        os.popen('sudo rm -rvf %s/*' % self.dirs['regsource'])




    def clearSubjectDirs(self):
        ''' deletes all files downloaded to or copied to the raw directory '''
        import os
        os.popen('sudo rm -rvf %s/*' % self.dirs['spec'])

        self._buildDirectoryStructure()


    def generateSourceVideo(self):

        cmdstr = '/usr/bin/avconv -f image2 -i %s/frame%%04d.jpg -r 12 -s hd1080 %s/source.mp4' % (self.dirs['regsource'], self.dirs['video'])
        print cmdstr
        pipe = os.popen(cmdstr, 'r')

        for p in pipe:
            print(p)

    def generateRegisteredVideo(self):

        cmdstr = '/usr/bin/avconv -f image2 -i %s/register%%04d.jpg -r 12 -s hd1080 %s/register.mp4' % (self.dirs['regtarget'], self.dirs['video'])
        print cmdstr
        pipe = os.popen(cmdstr, 'r')
        for p in pipe:
            print(p)

    



