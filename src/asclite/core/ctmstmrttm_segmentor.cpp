/*
 * asclite
 * Author: Jerome Ajot
 *
 * This software was developed at the National Institute of Standards and Technology by
 * employees of the Federal Government in the course of their official duties.  Pursuant to
 * Title 17 Section 105 of the United States Code this software is not subject to copyright
 * protection within the United States and is in the public domain. asclite is
 * an experimental system.  NIST assumes no responsibility whatsoever for its use by any party.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS."  With regard to this software, NIST MAKES NO EXPRESS
 * OR IMPLIED WARRANTY AS TO ANY MATTER WHATSOEVER, INCLUDING MERCHANTABILITY,
 * OR FITNESS FOR A PARTICULAR PURPOSE.
 */

/**
 * Implementation of a segmentor that take rttm hyp and rttm reference.
 */

#include "ctmstmrttm_segmentor.h" // class's header file

Logger* CTMSTMRTTMSegmentor::logger = Logger::getLogger();

// class destructor
CTMSTMRTTMSegmentor::~CTMSTMRTTMSegmentor()
{
	sourceList.clear();

	map<string, set<string> >::iterator i, ei;
	
	i = channelList.begin();
	ei = channelList.end();
	
	while(i != ei)
	{
		i->second.clear();
		++i;
	}
  
	channelList.clear();
}

void CTMSTMRTTMSegmentor::Reset(SpeechSet* references, SpeechSet* hypothesis)
{
    if (references != refs)
    {
        refs = references;
        //init the source and Channel list
        sourceList.clear();
        channelList.clear();
    
        for (size_t i=0 ; i < references->GetNumberOfSpeech(); ++i)
        {
            for (size_t j=0 ; j < references->GetSpeech(i)->NbOfSegments() ; ++j)
            {
                Segment* seg = references->GetSpeech(i)->GetSegment(j);
                sourceList.insert(seg->GetSource());
                channelList[seg->GetSource()].insert(seg->GetChannel());
            }
        }
    }
	
    hyps = hypothesis;
    //init the iterator
    currentSource = *(sourceList.begin());
    sourceList.erase(currentSource);
    currentChannel = *(channelList[currentSource].begin());
    LOG_INFO(logger, "Prepare to process source("+currentSource+") channel("+currentChannel+")");
    channelList[currentSource].erase(currentChannel);
    currentSegmentRef = GetFirstSegment(0, references);
}

SegmentsGroup* CTMSTMRTTMSegmentor::Next()
{
	SegmentsGroup* segGroup = new SegmentsGroup();

    // Retrieve time of the next blank
    Segment* lastOverlapingSeg = GetLastOverlapingSegment(currentSegmentRef->GetStartTime(), refs);
    //cout << "from:" << currentSegment->GetStartTime() << endl;
    //cout << "los seg("<< lastOverlapingSeg->GetStartTime() << "," << lastOverlapingSeg->GetEndTime() << ")" << endl;
    //cout << "Nbr of Speeches: " << refs->GetNumberOfSpeech() << endl;

    // Retrieve segments between current time and next blank
    for (size_t i=0 ; i < refs->GetNumberOfSpeech() ; ++i)
	{
        vector<Segment*> v_segs = refs->GetSpeech(i)->GetSegmentsByTime(currentSegmentRef->GetStartTime(), lastOverlapingSeg->GetEndTime(), currentSource, currentChannel);
        //cout << "nb of ref: " << v_segs.size() << endl;
        if (v_segs.size() != 0)
            segGroup->AddReference(v_segs);
    }
  
    for (size_t i=0 ; i < hyps->GetNumberOfSpeech(); ++i)
	{
        vector<Segment*> v_segs = hyps->GetSpeech(i)->GetSegmentsByTime(currentSegmentRef->GetStartTime(), lastOverlapingSeg->GetEndTime(), currentSource, currentChannel);
        //cout << "nb of hyp: " << v_segs.size() << endl;
        if (v_segs.size() != 0)
            segGroup->AddHypothesis(v_segs);
    }
  
    //Prepare the next iteration
    //cout << lastOverlapingSeg->GetEndTime() << endl;
    currentSegmentRef = GetFirstSegment(lastOverlapingSeg->GetEndTime(), refs);
	
    //cout << "cur seg("<< currentSegment->GetStartTime() << "," << currentSegment->GetEndTime() << ")" << endl;
    if (currentSegmentRef == NULL)
    {
        if (!channelList[currentSource].empty())
        {
            currentChannel = *(channelList[currentSource].begin());
            LOG_INFO(logger, "Prepare to process source("+currentSource+") channel("+currentChannel+")");
            channelList[currentSource].erase(currentChannel);
            currentSegmentRef = GetFirstSegment(0, refs);   
        } 
        else
        {
            if (!sourceList.empty())
            {
                currentSource = *(sourceList.begin());
                sourceList.erase(currentSource);
                currentChannel = *(channelList[currentSource].begin());
                channelList[currentSource].erase(currentChannel);
                LOG_INFO(logger, "Prepare to process source("+currentSource+") channel("+currentChannel+")");
                currentSegmentRef = GetFirstSegment(0, refs);
            }
            /*
            else
            {
                // currentSegment NULL mean end of loop in this case
            }
            */
        }
    }
	    
    return segGroup;
}

Segment* CTMSTMRTTMSegmentor::GetLastOverlapingSegment(int startTime, SpeechSet* speechs)
{
    //find the next time when there is no segments
    Segment* last = currentSegmentRef;
    bool again = true;
    
    while (again)
    {
        again = false;
        
        for (size_t i=0 ; i < speechs->GetNumberOfSpeech() ; ++i)
        {
            //cout << "get next seg start in : " << last->GetEndTime() << endl;
            Segment* t_seg = speechs->GetSpeech(i)->NextSegment(last->GetEndTime(), currentSource, currentChannel);
            
            if (t_seg != NULL)
            {
                //cout << "got seg start on " << t_seg->GetStartTime() << endl;
               if (t_seg->GetStartTime() < last->GetEndTime())
                {
                    last = t_seg;
                    again = true;
                }
            }
        }
    }
    
    return last;
}

Segment* CTMSTMRTTMSegmentor::GetFirstSegment(int startTime, SpeechSet* speechs)
{
    int min_time = INT_MAX;
	Segment* retSegment = NULL;
	
    for (size_t i=0 ; i < speechs->GetNumberOfSpeech() ; ++i)
	{
        Segment* t_seg = speechs->GetSpeech(i)->NextSegment(startTime, currentSource, currentChannel);
        
        if (t_seg != NULL)
        {
            if (t_seg->GetStartTime() < min_time)
            {
                retSegment = t_seg;
                min_time = t_seg->GetStartTime(); 
            }
        }
    }
		
    return retSegment;
}