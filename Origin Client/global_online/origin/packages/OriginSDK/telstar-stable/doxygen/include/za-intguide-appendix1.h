/// \page za-intguide-appendix1 Appendix: Multiple Instance Testing
/// \brief This configuration allows you to run multiple instances of Origin and multiple games
/// on the same computer, without having the games connect to the same default port.
/// 
/// <img alt="Configuring Multiple Instances of Origin" title="Configuring Multiple Instances of Origin" src="Appendix-SpecifyingPorts.png"/>
///
/// The default port that Origin-launched games connect to for communication with Origin is port 3216. To
/// override this behavior, the port can be specified as a commandline parameter in the following form:
/// <CODE>Origin.exe /LsxPort:<I>nnnn</I> -Origin_MultipleInstances</CODE>
///
/// The procedure to run multiple instances of Origin and multiple games on the same computer is as
/// follows:
/// <OL>
///     <LI>Start the first instance of Origin from commandline with a specified port, for example:<BR>
///         <CODE>Origin.exe /LsxPort:1491 -Origin_MultipleInstances</CODE><BR>
///         <I>Note that the port shown is an example only; any port number can be used, although
///         you should avoid
///         <A HREF="https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.txt">port numbers</A>
///         with which there may be conflicts.</I></LI>
///     <LI>Log in to Origin as user 1.</LI>
///     <LI>Initialize the Origin SDK:
///         <PRE>    OriginErrorT ORIGIN_SDK_API OriginStartup (int32_t  	        flags,
///                                            uint16_t  	        lsxPort,
///                                            OriginStartupInputT *  	pInput,
///                                            OriginStartupOutputT *  	pOutput 
/// )</PRE>
/// And set the lsxPort to the same number as you used to start the client, in this example, 1491.</LI>
///     <LI>Start the second instance of Origin from commandline with a different specified port, for example:<BR>
///         <CODE>Origin.exe /LsxPort:1528 -Origin_MultipleInstances</CODE><BR>
///         <I>Note that the port shown is an example only; any port number can be used, although
///         you should avoid
///         <A HREF="https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.txt">port numbers</A>
///         with which there may be conflicts, and it must differ from the port used in step 1.</I></LI>
///     <LI>Log in to Origin as user 2.</LI>
///     <LI>Initialize the Origin SDK:
///         <PRE>    OriginErrorT ORIGIN_SDK_API OriginStartup (int32_t  	        flags,
///                                            uint16_t  	        lsxPort,
///                                            OriginStartupInputT *  	pInput,
///                                            OriginStartupOutputT *  	pOutput 
/// )</PRE>
/// And set the lsxPort to the same number as you used to start the client, in this example, 1528.</LI>
/// </OL>
/// 
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly
