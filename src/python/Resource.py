# -*- coding: utf-8 -*-





class oldResource(object):

    """docstring foResourcme"""

    def __init__(self, _name):

        self.name =  _name

        #static strings 
        self.API_listOfSpecimens = '''http://api.brain-map.org/api/v2/data/query.json?criteria=model::Specimen,rma::criteria,donor(products[abbreviation$eqHumanASD]),rma::options[num_rows$eq100]'''
        self.API_baseDetailString = '''http://human.brain-map.org/api/v2/data/Specimen/%d.json?wrap=true&include=donor(age,conditions),data_sets(products[id$in9,10,11,26,27],genes,treatments),specimen_images,specimen_types,well_known_files,structure'''
        self.API_listOfImages = '''http://api.brain-map.org/api/v2/data/SectionDataSet/%d.json?include=section_images'''
        self.TILE_baseURL = '''http://human.brain-map.org/tiles//'''
        self.TILE_thumbnail = '''/TileGroup/0-0-0.jpg'''
        self.API_imageServiceBase = '''http://api.brain-map.org/cgi-bin/imageservice?path='''

        print 'Initialized as %s resource' % (self.name)

        #import SectionImage        
        #self.sectionImage = SectionImage.SectionImage('test')


    def generateThumbnailURL(self, series_image, siTop, siLeft, siWidth, siHeight):
        returnStr = self.TILE_baseURL + series_image + self.TILE_thumbnail
        returnStr += '?siTop=' + siTop
        returnStr += '&siLeft=' + siLeft 
        returnStr += '&siWidth=' + siWidth
        returnStr += '&siHeight=' + siHeight
        return returnStr

    
    def generateDownSampleURL(self, series_image, siTop, siLeft, siWidth, siHeight, downsample):
        import math
        returnStr = self.API_imageServiceBase + series_image
        returnStr += '&top=' + str(int(siTop / math.pow(2,downsample))) 
        returnStr += '&left=' + str(int(siLeft / math.pow(2,downsample))) 
        returnStr += '&width=' + str(int(siWidth / math.pow(2,downsample))) 
        returnStr += '&height=' + str(int(siHeight / math.pow(2,downsample))) 
        returnStr += '&downsample=' + str(downsample)
        return returnStr


    def getListOfAutism(self):
        import requests
        r = requests.get(self.API_listOfSpecimens)
        if r.status_code == 200:
            listOfSpecimens = r.json()['msg']
            return listOfSpecimens
        else:
            return []


    def filterSubjectList(self, subjectList, subjectName):

        specimen_of_interest = []
        for specimenToCopy in subjectList:

            keys_needed = ['id', 'donor_id', 'name', 'rna_integrity_number', 'structure_id']
            specimen = {}
            for key in keys_needed:
                specimen[key] = specimenToCopy[key]
                
            if subjectName == specimen['name']:
                specimen_of_interest.append(specimen)

        return specimen_of_interest



    def getSubjectDetails(self, subjectID):
        specimenDetailJSON = self.API_baseDetailString % subjectID
        d = requests.get(specimenDetailJSON)

        if d.status_code == 200:
            return d.json()['msg'][0]['data_sets']

        else:
            return []

    def filterSubjectDetailsByMarkerList(self, subjectDetails, markerList):

        specimenMarkerSet = []
        specimenFilteredMarkerSet = []

        # markerList = ['RORB', 'CALB1', 'NISSL']

        for ds in subjectDetails:
            
            geneSingle = {}
            
            if ds['treatments'][0]['tags'] == 'histology':
                #print '%d - HIS: %s' % (ds['id'], ds['treatments'][0]['name'])
                
                geneSingle['id'] = ds['id']
                geneSingle['type'] = 'HIS'
                geneSingle['name'] = ds['treatments'][0]['name']
                
            elif ds['treatments'][0]['tags'] == 'In Situ Hybridization histology':
                
                for gene in ds['genes']:
                    #print '%d - ISH: %s - %s' % (ds['id'], gene['acronym'], gene['name']) 
                    
                    geneSingle['id'] = ds['id']
                    geneSingle['type'] = 'ISH'
                    geneSingle['name'] = gene['acronym']
            
            specimenMarkerSet.append(geneSingle)
            
            if geneSingle['name'] in markerList:
                specimenFilteredMarkerSet.append(geneSingle)

        return specimenFilteredMarkerSet


    def getSeriesGallery(self, series_id):
        
        requestURL = self.API_listOfImages % series_id
        seriesImageData = requests.get(requestURL).json()['msg']
        galleryString = self.generateGalleryString(seriesImageData[0]['section_images'], sorted=True)
        return galleryString

    def generateGalleryString(self, series_image_list, sorted=False):
        # sort by section number, ascending
        
        gallery_str = ''
        
        if sorted == True:
            import operator
            series_image_list.sort(key=operator.itemgetter('section_number'))
            
        for seriesImg in series_image_list:
            
            _siTop = str(seriesImg['y'])
            _siLeft = str(seriesImg['x'])
            _siHeight = str(seriesImg['height'])
            _siWidth = str(seriesImg['width'])
            _path = seriesImg['path']
            thumbnail = self.generateThumbnailURL(_path, _siTop, _siLeft, _siWidth, _siHeight)
            sec_num = seriesImg['section_number']
            
            imgstr = '''<div style="float:left; height:140px;"><div>Section number: <strong>%d</strong></div><img style="border:1px solid #333; margin:5px; width:128px;" src='%s'/></div>''' % (sec_num, thumbnail)
            gallery_str += imgstr
            
        return gallery_str


    def getImageDictForID(self,series_id): 
        dict_to_return = {}
        requestURL = self.API_listOfImages % series_id
        section_images = requests.get(requestURL).json()['msg'][0]['section_images']
        for image in section_images:
            dict_to_return[image['section_number']] = image
        return dict_to_return

    def getImageListForID(self,series_id):
        requestURL = self.API_listOfImages % series_id
        list_to_return = requests.get(requestURL).json()['msg'][0]['section_images']
        return list_to_return

    def generateSeriesImageList(self, filteredMarkerList, sorted=False):
        ''' gets a list of all images contained in the marker list'''
        allImageList = []

        for f in filteredMarkerList:
            allImageList += self.getImageListForID(f['id'])

        if sorted == True:
            import operator
            allImageList.sort(key=operator.itemgetter('section_number'))

        return allImageList


















