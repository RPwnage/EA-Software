pb2xml
-------------

The pb2xml project is designed to output Google protobuf messages to xml.

**C++**

The C++ method "xml\_format.cc" borrows _heavily_ on the text\_format.cc class defined at the google code repository:
[text\_format.cc](http://code.google.com/apis/protocolbuffers/docs/reference/cpp/google.protobuf.text_format.html).


[RapidXML](http://rapidxml.sourceforge.net/) was choosen to encode the XML protobuf DOM object. Alternate xml libraries could be used (just check the source and replace with library of your choice).

To convert the protobuf object into an xml `rapidxml::xml_document<>` document object use the `void XmlFormat::Printer::MessageToDOM(const Message& message, rapidxml::xml_document<>* doc)` method. The method `void XmlFormat::Printer::PrintToXmlString(const Message& message, string* output)` modifies the string `output` with the contents of the generated DOM object.


Given an instance of the protobuf example message definition, `addressbook.proto` as the variable `tutorial::AddressBook address_book`:

> `$ cout << address_book.DebugString() << endl;`  
> <code>person {  
  name: "name1"  
  id: 12  
  email: "asdf@example.com"  
  phone {  
    number: "222203333"  
    type: WORK  
  }  
}  
person {  
  name: "Second Name"  
  id: 2  
}
</code>

And the equivalent in XML:

> <code> $ string debug_string;  
$ google::protobuf::XmlFormat::PrintToXmlString(address_book, &debug_string);  
$ cout &lt;&lt; debug_string &lt;&lt; endl;`   
</code>

> <code>   &lt;?xml version="1.0" encoding="utf-8"?&gt;  
    &lt;AddressBook&gt;  
    	&lt;person&gt;  
      		&lt;name&gt;name1&lt;/name&gt;  
      		&lt;id&gt;12&lt;/id&gt;  
      		&lt;email&gt;asdf@example.com&lt;/email&gt;
      		&lt;phone&gt;  
      			&lt;number&gt;222203333&lt;/number&gt;  
      			&lt;type&gt;WORK&lt;/type&gt;  
      		&lt;/phone&gt;  
      	&lt;/person&gt;  
      	&lt;person&gt;  
      		&lt;name&gt;Second Name&lt;/name&gt;  
      		&lt;id&gt;2&lt;/id&gt;  
      	&lt;/person&gt;  
      &lt;/AddressBook&gt;  

</code>


Note that other RapidXML printing methods can be used, including removing the whitespace from the xml using the `print_no_indenting` on the DOM object.

Also if the user wants to output the xml directly (rather than using a DOM model to contain the object heirarchy, you need to write your own [TextGenerator](http://groups.google.com/group/protobuf/browse_thread/thread/8a1a2ffd02b2a488/722eb96c115d859e?hl=en&lnk=gst&q=marthaler&pli=1) class as per the `text_format.cc`.


