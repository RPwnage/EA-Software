(function($){
"use strict";

$.fn.chatInput = function() 
{
    var $elm = $(this),
        maxMessageBytes = originChat.maximumOutgoingMessageBytes;

    // Handle keydown events
    $elm.on('keydown', function(evt)
    {
        window.setTimeout(function()
        {
            var body = $elm.text();
            
            if (body.length === 0)
            {
                $elm.trigger('stopComposing');
                return;
            }
            else
            {
                $elm.trigger('composing');
            }
        }, 0);
        
        // Look for enter pressed without Shift
        if ((evt.keyCode !== 13) || (evt.shiftKey === true))
        {
            // Propagate the event
            return true;
        }

        // Our content isn't updated yet - set a 0ms timeout for it to update
        window.setTimeout(function()
        {
            var body = $elm.text();
            
            if (body.length === 0)
            {
                // Do nothing
                return;
            }

            // Using $elm.empty() causes QtWebKit 2.2 to get in a weird state 
            // where the elment is still focused but the text entry caret is 
            // gone. This appears to be fixed in or before Chrome 23.
            document.execCommand('selectAll');
            document.execCommand('delete');

            // Create our chat input event
            $elm.trigger('chatInput', [body]);
        }, 0);
        
        return false;
    });

    if (maxMessageBytes !== null)
    {
        // Enforce our maximum outgoing message size
        $elm.on('input', function (evt)
        {
            var textNode, range, bytesLeft, nodeIdx, textIdx, codePoint,
                nodeText;

            // Since RFC-3629 the longest UTF-8 byte sequence is four bytes
            // If the text would still fit under the max message bytes in the
            // worst case skip the expensive byte counting below
            if ($(this).text() <= Math.floor(maxMessageBytes / 4))
            {
                return;
            }
            
            bytesLeft = maxMessageBytes;

            // Iterate over the child nodes until we run out of bytes
            for(nodeIdx = 0; nodeIdx < this.childNodes.length; nodeIdx++)
            {
                textNode = this.childNodes[nodeIdx];
                nodeText = textNode.textContent;
                
                for(textIdx = 0; textIdx < nodeText.length; textIdx++)
                {
                    codePoint = nodeText.charCodeAt(textIdx);

                    // Determine the length of the UTF-8 byte sequence for this
                    // code point. Unfortunately there's no clean way to do this
                    // using the JavaScript standard library
                    if (codePoint > 0x10000) 
                    {
                        bytesLeft -= 4;
                    }
                    else if (codePoint > 0x800)
                    {
                        bytesLeft -= 3;
                    }
                    else if (codePoint > 0x80)
                    {
                        bytesLeft -= 2;
                    }
                    else
                    {
                        bytesLeft -= 1;
                    }

                    if (bytesLeft < 0)
                    {
                        // Slice the extra bytes off this node
                        range = document.createRange();
                        range.selectNode(textNode);
                        range.setStart(textNode, textIdx);
                        range.deleteContents();

                        // Remove all nodes afterwards
                        while(this.childNodes.length > (nodeIdx + 1))
                        {
                            this.removeChild(this.childNodes[nodeIdx + 1]);
                        }

                        // We're done
                        return;
                    }
                }
            }
        });
    }

    return this;
};

}(jQuery));
