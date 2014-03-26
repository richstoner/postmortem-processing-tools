# -*- coding: utf-8 -*-

class SectionImage(object):
    """docstring for SectionImage"""

    def __init__(self, _tag, remote=True):
        self.tag =  _tag # tag corresponds to section number, specimen - experiment
        self.metadata = {}
        self.section_number = -1

        self.remote = remote
        self.localCacheDir = ''

        # not really sure how to break these up
        self.TILE_baseURL = '''http://human.brain-map.org/tiles//'''
        self.TILE_thumbnail = '''/TileGroup/0-0-0.jpg'''
        self.API_imageServiceBase = '''http://api.brain-map.org/cgi-bin/imageservice?path='''

        # see: http://help.brain-map.org/display/api/Downloading+an+Image

        # print 'Initialized as SectionImage (tag: %s)' % (self.tag)

    def __str__(self):
        return '%s - [ %s x %s ]' % (self.tag, self.metadata['width'], self.metadata['height'])

    def __repr__(self):
        return '%s - [ %s x %s ]' % (self.tag, self.metadata['width'], self.metadata['height'])
        
# useful if remote
    def generateThumbnailURL(self):
        returnStr = self.TILE_baseURL + self.metadata['path'] + self.TILE_thumbnail
        returnStr += '?siTop=' + self.metadata['y']
        returnStr += '&siLeft=' + self.metadata['x']
        returnStr += '&siWidth=' + self.metadata['width']
        returnStr += '&siHeight=' + self.metadata['height']
        return returnStr

# useful if remote    
    def generateDownSampleURL(self, downsample):
        import math
        returnStr = self.API_imageServiceBase + self.metadata['path']
        returnStr += '&top=' + str(int(self.metadata['y']))
        returnStr += '&left=' + str(int(self.metadata['x']))
        returnStr += '&width=' + str(int(self.metadata['width'] / math.pow(2,downsample))) 
        returnStr += '&height=' + str(int(self.metadata['height'] / math.pow(2,downsample))) 
        returnStr += '&downsample=' + str(downsample)
        return returnStr

#useful if local
    def generateThumbnailConversion(self):

        if self.remote == True:
            return

        commandString = '/home/ubuntu/external/kakadu/kdu_expand -i %s' % self.metadata['path']

        return 'this returns the path to the thumbnail'

    def generateDownSampleConversion(self, path, ds=5):
        import os
        if self.remote == True:
            return

        outputname = '%s/%s-DSx%d' % (path, self.tag, ds) #shortened tag 
        print outputname + '.jpg'

        if not os.path.exists(outputname+ '.jpg' ):

            commandString = '/home/ubuntu/external/kakadu/kdu_expand -i %s -o %s.tif -reduce %d' % (self.metadata['path'], outputname, ds)
            #print commandString
            p = os.popen(commandString)
            print(p.read())

            commandString = '/usr/bin/convert %s.tif %s.jpg' % (outputname, outputname)
            #print commandString
            p = os.popen(commandString)     
            print(p.read())

            commandString = 'rm -vrf %s.tif' % (outputname)
            #print commandString
            p = os.popen(commandString)     
            print(p.read())

        return outputname + '.jpg'



    def getImageDetailsFromJP2(self):
        ''' populates the image metadata from the jp2 image using kakadu jp2info'''

        if self.remote == True:
            return

        import os
        from bs4 import BeautifulSoup

        commandString = '/home/ubuntu/external/kakadu/kdu_jp2info -i %s' % self.metadata['path']
        p = os.popen(commandString)
        xmlinfo = BeautifulSoup(p.read())
        w = int(xmlinfo.find_all('width')[0].string)
        h = int(xmlinfo.find_all('height')[0].string)
        
        self.metadata['width'] = w
        self.metadata['height'] = h








class Specimen(object):
    ''' the generic AIBS specimen object'''

    def __init__(self, initdata={},remote=True, subjectName='undefined'):

        self.remoteSpecimen = remote
        self.metadata = {}
        self.sectionImageList = []
        self.rawImageList = []
        self.markersOfInterest = []
        self.allMarkers = []
        self.filteredMarkers = []
        self.localMarkers = []
        self.localImageList = []
        self.sectionRange = []

        if len(initdata) > 0:
            self.subjectName = initdata['subjectName'] # tentative
        else:
            self.subjectName = subjectName
        
        if remote:
            self.imageLocation = 'api'

        if not remote:

            self.imageLocation = initdata['location']

            import glob
            self.rawImageList = []
            allRawImagesInLocation = glob.glob(self.imageLocation + '*/*.jp2')
            # print allRawImagesInLocation

            for ri in allRawImagesInLocation:
                if self.subjectName in ri:
                    self.rawImageList.append(ri)

            print 'found %d images for Subject %s' % (len(self.rawImageList), self.subjectName)


    def populateFromLocalImages(self):


        self.localMarkers = []
        self.localImageList = []
        self.sectionImageList = []

        if self.remoteSpecimen:
            print 'not valid'
            return
        
        print 'populating from local file data'

        # look for csv with information? 
        # 
        bFoundCSVlut = False

        if bFoundCSVlut:
            print 'loading from CSV'

        # if no csv, try to extrapolate from file name

        subjectNameList = []

        import os
        for img in self.rawImageList:

            pathname = os.path.split(img)[0].split('/')[-1].split('_')
            marker_name = pathname[4]



            basename = os.path.splitext(os.path.basename(img))[0].split('_')
            
            fields = ['study_id', 'subjectName', 'section_number', 'specimen_id', 'experiment_id', 'ds', 'quality']

            sInfo = {}
            sInfo['path'] = img

            for n,f in enumerate(fields):
                if f == 'section_number':
                    sInfo[fields[n]] = int(basename[n].replace('p', ''))
                elif f == 'specimen_id':
                    sInfo[fields[n]] = int(basename[n].replace('s', '')) 
                elif f == 'experiment_id':
                    # actually the barcode
                    sInfo[fields[n]] = int(basename[n].replace('b', '')) 
                elif f == 'subjectName':

                    if basename[n] not in subjectNameList:
                        subjectNameList.append(basename[n])

                else:
                    sInfo[fields[n]] = basename[n]


            if len(self.sectionRange) == 2:

                # need to set metadata for image here
                tag = '%03d-%s' % (sInfo['section_number'], sInfo['specimen_id'])
                
                if sInfo['section_number'] >= self.sectionRange[0] and sInfo['section_number'] <= self.sectionRange[1]:

                    s = SectionImage(tag, remote=False)
                    s.metadata = sInfo
                    s.section_number = sInfo['section_number']

                    s.getImageDetailsFromJP2()
                    self.localImageList.append(s)

                    geneSingle = {}
                    geneSingle['id'] = sInfo['specimen_id'] # s number

                    if len(pathname) > 5:
                        geneSingle['type'] = 'ISH'
                    else:
                        geneSingle['type'] = 'HIS'

                    geneSingle['name'] = marker_name

                    if geneSingle not in self.localMarkers:
                        self.localMarkers.append(geneSingle)
                else:
                    pass
                    # print 'section %s not in range %s, skip' % (sInfo['section_number'], self.sectionRange)

            else:

                # need to set metadata for image here
                tag = '%03d-%s' % (sInfo['section_number'], sInfo['specimen_id'])
                s = SectionImage(tag, remote=False)
                s.metadata = sInfo
                s.section_number = sInfo['section_number']

                s.getImageDetailsFromJP2()
                self.localImageList.append(s)

                geneSingle = {}
                geneSingle['id'] = sInfo['specimen_id'] # s number

                if len(pathname) > 5:
                    geneSingle['type'] = 'ISH'
                else:
                    geneSingle['type'] = 'HIS'

                geneSingle['name'] = marker_name

                if geneSingle not in self.localMarkers:
                    self.localMarkers.append(geneSingle)


        print subjectNameList

        if len(subjectNameList) > 1:
            print 'error - multiple subjects found in this list'
            
        else:
            self.subjectName = subjectNameList[0]
            print 'Setting specimen name to %s' % (self.subjectName)



        
            

    def getListOfAvailableMarkers(self):

        if self.remoteSpecimen:

            import requests
            API_baseDetailString = '''http://human.brain-map.org/api/v2/data/Specimen/%d.json?wrap=true&include=donor(age,conditions),data_sets(products[id$in9,10,11,26,27],genes,treatments),specimen_images,specimen_types,well_known_files,structure'''
            specimenDetailJSON = API_baseDetailString % self.metadata['id']
            d = requests.get(specimenDetailJSON)

            if d.status_code == 200:
                if type(d.json) == type(dict()):
                    if d.json['num_rows'] > 0:
                        return d.json['msg'][0]['data_sets']
                else:
                    if d.json()['num_rows'] > 0:
                        return d.json()['msg'][0]['data_sets']
            else:
                return []

        else:
            print 'not implemented yet'


    def getMarkerList(self, verbose=True):
        ''' returns marker list, filtered if wanted'''

        if self.remoteSpecimen:

            details = self.getListOfAvailableMarkers()
            if type(details) == type(None):
                if verbose:
                    print 'no markers found'
                return

            specimenMarkerSet = []
            specimenFilteredMarkerSet = []

            for ds in details:
                
                geneSingle = {}
                
                if ds['treatments'][0]['tags'] == 'histology':
                    if verbose:
                        print '%d - HIS: %s' % (ds['id'], ds['treatments'][0]['name'])
                    
                    geneSingle['id'] = ds['id']
                    geneSingle['type'] = 'HIS'
                    geneSingle['name'] = ds['treatments'][0]['name']
                    
                elif ds['treatments'][0]['tags'] == 'In Situ Hybridization histology':
                    
                    for gene in ds['genes']:
                        if verbose:
                            print '%d - ISH: %s - %s' % (ds['id'], gene['acronym'], gene['name']) 
                        
                        geneSingle['id'] = ds['id']
                        geneSingle['type'] = 'ISH'
                        geneSingle['name'] = gene['acronym']
                
                specimenMarkerSet.append(geneSingle)

                if geneSingle['name'] in self.markersOfInterest:
                    specimenFilteredMarkerSet.append(geneSingle)

                self.filteredMarkers = specimenFilteredMarkerSet
                self.allMarkers = specimenMarkerSet
        
        else:

            specimenMarkerSet = []
            specimenFilteredMarkerSet = []

            
            # local specimen
            for marker in self.localMarkers:
                if marker['name'] in self.markersOfInterest:
                    specimenFilteredMarkerSet.append(marker)

                specimenMarkerSet.append(marker)
        

            self.filteredMarkers = specimenFilteredMarkerSet
            self.allMarkers = specimenMarkerSet            

            if verbose == True:
                print self.allMarkers


    def getSectionImages(self, onlyFiltered=True):
        if self.remoteSpecimen:

            self.sectionImageList = []
            
            list_to_use = []
            if onlyFiltered and len(self.filteredMarkers) > 0:
                list_to_use = self.filteredMarkers
            else:
                list_to_use = self.allMarkers

            for sds in list_to_use:
                import aibs
                api = aibs.api()

                imageList = api.getSectionImagesForID(sds['id'])
                self.sectionImageList += imageList

            return self.sectionImageList            

        else:
            self.sectionImageList = []
            

            list_to_use = []
            if onlyFiltered and len(self.filteredMarkers) > 0:
                list_to_use = self.filteredMarkers
            else:
                list_to_use = self.allMarkers

            for sds in list_to_use:
                imageList = self._getLocalImageList(sds['id'])
                #imageList = api.getSectionImagesForID(sds['id'])
                self.sectionImageList += imageList

            return self.sectionImageList            
            
    def _getLocalImageList(self, id_needed):

        list_to_return = []
        for possible_image in self.localImageList:

            # already section images
            if str(id_needed) in possible_image.tag:
                list_to_return.append(possible_image)

        return list_to_return
            



    def getSortedImageList(self):
        import operator
        list_to_sort = self.sectionImageList
        list_to_sort.sort(key=operator.attrgetter('section_number'))
        return list_to_sort


    def printSpecimenDetails(self):

        import pprint
        print('all markers')
        pprint.pprint(self.allMarkers)

        print('filtered markers')
        pprint.pprint(self.filteredMarkers)

        print('section image list')
        pprint.pprint(self.sectionImageList)












import requests

class api(object):

    def __init__(self):
        self.API_listOfSpecimens = '''http://api.brain-map.org/api/v2/data/query.json?criteria=model::Specimen,rma::criteria,donor(products[abbreviation$eqHumanASD]),rma::options[num_rows$eq100]'''
        self.API_baseDetailString = '''http://human.brain-map.org/api/v2/data/Specimen/%d.json?wrap=true&include=donor(age,conditions),data_sets(products[id$in9,10,11,26,27],genes,treatments),specimen_images,specimen_types,well_known_files,structure'''
        self.API_listOfImages = '''http://api.brain-map.org/api/v2/data/SectionDataSet/%d.json?include=section_images'''
        self.TILE_baseURL = '''http://human.brain-map.org/tiles//'''
        self.TILE_thumbnail = '''/TileGroup/0-0-0.jpg'''
        self.API_imageServiceBase = '''http://api.brain-map.org/cgi-bin/imageservice?path='''



    def getSpecimensWithName(self, sname):

        specimen_of_interest = []

        for specimenToCopy in self._getListOfAutism():
            if sname in specimenToCopy['name']:
                s = Specimen(remote=True, subjectName=sname)
                s.metadata = specimenToCopy
                specimen_of_interest.append(s)

        return specimen_of_interest

    def getValidSpecimentsWithName(self, sname):

        speclist = self.getSpecimensWithName(sname);

        explist = []
        for s in speclist:
            s.getMarkerList(verbose=False)
            if len(s.allMarkers) > 0:
                explist.append(s)

        return explist


    def _getListOfAutism(self):
        import requests
        r = requests.get(self.API_listOfSpecimens)
        if r.status_code == 200:
            return self._jsonhelper(r)

    def _jsonhelper(self, resp):
        list_to_return = []
        if type(resp.json) == type(dict()):
            if resp.json['num_rows'] > 0:
                list_to_return = resp.json['msg']
        else:
            if resp.json()['num_rows'] > 0:
                list_to_return = resp.json()['msg']
        return list_to_return
    


    def getImageListForID(self, series_id):
        requestURL = self.API_listOfImages % series_id
        r = requests.get(requestURL)

        seriesImageData = []
        if r.status_code == 200:
            seriesImageData = self._jsonhelper(r)


        if(type(seriesImageData) == type(list())):
            seriesImageData = seriesImageData[0]

        return seriesImageData


    def getSectionImagesForID(self, series_id, sorted=True):

        list_to_return = []

        sds = self.getImageListForID(series_id)
        sectionList = sds['section_images']

        if sorted == True:
            import operator
            sectionList.sort(key=operator.itemgetter('section_number'))


        for si in sectionList:
            tag = '%03d-%s' % (si['section_number'], series_id)

            s = SectionImage(tag)
            s.metadata = si
            s.section_number = si['section_number']

            list_to_return.append(s)

        return list_to_return


    def getDSImagesFromListToPath(self, imageList, _path, downsample=5, redownload=False):
        import urllib, os

        output_list = []

        for img in imageList:
            dsurl =  img.generateDownSampleURL(downsample)
            print dsurl
            #outputname = '%s/%s-%s-DSx%d.jpg' % (_path, img.tag, img.metadata['id'], ds)
            outputname = '%s/%s-DSx%d.jpg' % (_path, img.tag, downsample) #shortened tag

            if not os.path.exists(outputname) or redownload:
                urllib.urlretrieve(dsurl, outputname)
                output_list.append(outputname)
                #print 'downloaded: %s to %s' % (dsurl, outputname)
            else:
                #print 'exists  : %s' % (outputname)
                pass



        import glob
        files_in_path = glob.glob(_path + '/*')

        return files_in_path



