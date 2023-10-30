/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2016 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <zmq.hpp>
#include "DataThread.h"
#include "../SourceNode/SourceNode.h"
#include "../../Utils/Utils.h"

#include "../Editors/GenericEditor.h"

DataThread::DataThread(SourceNode* s_)
    : Thread("Data Thread"),
      sn(s_),
      context(1),
      socket(context, ZMQ_PUB)
{
    setPriority(10);
    socket.bind("tcp://*:5555");
}



DataThread::~DataThread()
{
}


void DataThread::run()
{
    while (!threadShouldExit())
    {
        // Get the current timestamp
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        
        // Convert the timestamp to a string
        std::string timestampStr = std::to_string(timestamp);
        
        // Send the timestamp via ZeroMQ
        zmq::message_t message(timestampStr.size());
        memcpy(message.data(), timestampStr.c_str(), timestampStr.size());
        socket.send(message);
        
        if (!updateBuffer())
        {
            const MessageManagerLock mmLock(Thread::getCurrentThread());
            LOGE("Aquisition error...stopping thread.");
            signalThreadShouldExit();
            LOGE("Notifying source node to stop acqusition.");
            sn->connectionLost();
        }
    }
}


DataBuffer* DataThread::getBufferAddress(int streamIdx) const
{
	return sourceBuffers[streamIdx];
}


std::unique_ptr<GenericEditor> DataThread::createEditor (SourceNode*)
{
    return nullptr;
}


void DataThread::broadcastMessage(String msg)
{
    sn->broadcastDataThreadMessage(msg);
}
