/**
 *  HttpEncoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.protocol;

import java.nio.CharBuffer;
import java.util.TreeMap;
import java.util.Stack;
import java.util.Vector;

import ea.blaze.protocol.Base64Encoder;
import ea.blaze.tdf.Bitfield;
import ea.blaze.tdf.BlazeObjectId;
import ea.blaze.tdf.BlazeObjectType;
import ea.blaze.tdf.Blob;
import ea.blaze.tdf.MapHelper;
import ea.blaze.tdf.TagInfo;
import ea.blaze.tdf.TagInfoMap;
import ea.blaze.tdf.Tdf;
import ea.blaze.tdf.TimeValue;
import ea.blaze.tdf.Union;
import ea.blaze.tdf.VariableTdfContainer;
import ea.blaze.tdf.VectorHelper;
import ea.blaze.tdf.types.TdfBoolean;
import ea.blaze.tdf.types.TdfChar;
import ea.blaze.tdf.types.TdfByte;
import ea.blaze.tdf.types.TdfEnum;
import ea.blaze.tdf.types.TdfFloat;
import ea.blaze.tdf.types.TdfInteger;
import ea.blaze.tdf.types.TdfLong;
import ea.blaze.tdf.types.TdfShort;
import ea.blaze.tdf.types.TdfString;
import ea.blaze.tdf.types.TdfUInt64;

/**
 * HttpEncoder encodes an HTTP parameter string from a TDF, to match the expected format
 * of the blaze Web Access Layer.
 */
public class HttpEncoder extends TdfEncoder {

	/** The output buffer. */
	private CharBuffer mParamBuffer = null;
	
	private boolean mResult = true;
	
	/** The tag info map for the current TDF. */
	private TagInfoMap mTagInfoMap = null;
	
	/**
	 * The state of the TDF traversal.
	 */
	private enum State { 
		 STATE_NORMAL, 
		 STATE_ARRAY, 
		 STATE_MAP, 
		 STATE_UNION 
	};
	
	/** The state stack for tracking TDF traversal state. */
	private Stack<State> mStateStack = new Stack<State>();
	
	/** The nested tags to build up HTTP parameter names */
	private Stack<String> mNestedTags = new Stack<String>();
	
	/** The active union member. */
	private int mActiveUnionMember = Union.INVALID_MEMBER_INDEX;
	
	/**
	 * MapState container to track map traversal state, whether 
	 * we are visiting a field or a key
	 */
	private class MapState {
		
		/** The next field is key. */
		private boolean mNextIsKey = true;
	}
	
	/** The map state stack. */
	private Stack<MapState> mMapStateStack = new Stack<MapState>();
	
	/**
	 * VectorState container to track vector traversal state, which
	 * current member of the vector we are visiting.
	 */
	private class VectorState {
		
		/** The vector index. */
		private int vectorIndex = 0;
	}
	
	/** The vector state stack. */
	private Stack<VectorState> mVectorStateStack = new Stack<VectorState>();
	
	/**
	 * Instantiates a new Http encoder.
	 */
	public HttpEncoder() {
		
	}
	
	/* (non-Javadoc)
	 * @see ea.blaze.protocol.Encoder#getName()
	 */
	@Override
	public String getName() {
		return "http";
	}

	/* (non-Javadoc)
	 * @see ea.blaze.protocol.Encoder#getType()
	 */
	@Override
	public Type getType() {
		return Type.HTTP;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf tdf, Tdf referenceValue) {
		mStateStack.push(State.STATE_NORMAL);
		TagInfoMap old = mTagInfoMap;
		mTagInfoMap = tdf.getTagInfoMap();
		mParamBuffer = mByteBuffer.asCharBuffer();
		tdf.visit(this, tdf, referenceValue);
		mParamBuffer = null;
		mTagInfoMap = old;
		mStateStack.pop();
		return mResult;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Union tdf, Union referenceValue) {
		mStateStack.push(State.STATE_UNION);
		TagInfoMap old = mTagInfoMap;
		mTagInfoMap = tdf.getTagInfoMap();
		mParamBuffer = mByteBuffer.asCharBuffer();
		mActiveUnionMember = tdf.getActiveMember();
		tdf.visit(this, tdf, referenceValue);
		mParamBuffer = null;
		mTagInfoMap = old;
		mStateStack.pop();
		return mResult;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.Vector, java.util.Vector, ea.blaze.tdf.VectorHelper)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Vector<?> value,
			Vector<?> referenceValue, VectorHelper memberVisitor) {
		
		if (tag == Tdf.CONTAINER_MEMBER && mStateStack.peek() == State.STATE_ARRAY) {
			int currentVectorIndex = mVectorStateStack.peek().vectorIndex;
			mNestedTags.push(Integer.toString(currentVectorIndex));
			mVectorStateStack.peek().vectorIndex++;
		}
		else
			mNestedTags.push(TagInfo.decodeTag(tag, false));

		mStateStack.push(State.STATE_ARRAY);
		mVectorStateStack.push(new VectorState());
		
		/* Write the header */
		String parentTagsStr = parentTagsStr();
		String header = String.format("%s|=%d&", parentTagsStr, value.size());
		addToRequestString(header);
		
		/* Write the members */
		memberVisitor.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
		mVectorStateStack.pop();
		mNestedTags.pop();
		mStateStack.pop();
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.TreeMap, java.util.TreeMap, ea.blaze.tdf.MapHelper)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag,
			TreeMap<?, ?> value, TreeMap<?, ?> referenceValue,
			MapHelper mapVisitor) {
		
		mStateStack.push(State.STATE_MAP);
		MapState state = new MapState();
		state.mNextIsKey = true;
		mNestedTags.push(TagInfo.decodeTag(tag, false));
		mMapStateStack.push(state);
		mapVisitor.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
		mMapStateStack.pop();
		mNestedTags.pop();
		mStateStack.pop();
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfBoolean, ea.blaze.tdf.types.TdfBoolean, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfBoolean value,
			TdfBoolean referenceValue, boolean defaultValue) {
		
		addToRequestString(tag, value.toString());
	}
    
    /* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfChar, ea.blaze.tdf.types.TdfChar, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfChar value,
			TdfChar referenceValue, char defaultValue) {

            addToRequestString(tag, value.toString());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfByte, ea.blaze.tdf.types.TdfByte, byte)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfByte value,
			TdfByte referenceValue, byte defaultValue) {
		
		addToRequestString(tag, value.toString());

	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfShort, ea.blaze.tdf.types.TdfShort, short)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfShort value,
			TdfShort referenceValue, short defaultValue) {
		
		addToRequestString(tag, value.toString());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, int)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, int defaultValue) {
		
		addToRequestString(tag, value.toString());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfLong, ea.blaze.tdf.types.TdfLong, long)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfLong value,
			TdfLong referenceValue, long defaultValue) {

		addToRequestString(tag, value.toString());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfUInt64 value,
			TdfUInt64 referenceValue, TdfUInt64 defaultValue) {

		addToRequestString(tag, value.toString());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfFloat, ea.blaze.tdf.types.TdfFloat, float)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfFloat value,
			TdfFloat referenceValue, float defaultValue) {

		addToRequestString(tag, value.toString());

	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Bitfield, ea.blaze.tdf.Bitfield)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Bitfield value,
			Bitfield referenceValue) {
		
		addToRequestString(tag, Long.toString(value.getBits()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfString, ea.blaze.tdf.types.TdfString, java.lang.String, long)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfString value,
			TdfString referenceValue, String defaultValue, long maxLength) {

		addToRequestString(tag, value.toString());

	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Blob, ea.blaze.tdf.Blob)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Blob value,
			Blob referenceValue) {

		Base64Encoder e = new Base64Encoder();
		String s = e.encode(value.getData());
		
		/* Replace special characters */
		String procString = new String();
		for (int i = 0; i < s.length(); i++) {
			char c = s.charAt(i);
			switch (c) {
			case '+':
				procString += "%2B";
				break;
			case '=':
				procString += "%3D";
				break;
			case '/':
				procString += "%2F";
				break;
			default:
				procString += c;
				break;
			}
		}
		
		addToRequestString(tag, procString);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, java.lang.Class, java.lang.Enum)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value, 
			TdfInteger referenceValue, TdfEnum enumHelper, TdfEnum defaultValue) {
		
	    addToRequestString(tag, value.toString());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, Tdf value,
			Tdf referenceValue) {
		
		TagInfoMap old = mTagInfoMap;
		mTagInfoMap = value.getTagInfoMap();
		
		if (mStateStack.peek() == State.STATE_ARRAY) {
			int currentVectorIndex = mVectorStateStack.peek().vectorIndex;
			mNestedTags.push(Integer.toString(currentVectorIndex));
			mVectorStateStack.peek().vectorIndex++;
		}
		else if (mStateStack.peek() == State.STATE_MAP) {
			// Empty: not pushing any nested tags.
			// For maps, the keys are pushed when they
			// are encountered. If we're here, we know
			// that a key has already been pushed.
		}
		else if(mStateStack.peek() == State.STATE_UNION) {
			mNestedTags.push(old.getTagAtIndex(mActiveUnionMember).getMember());
		}
		else
			mNestedTags.push(TagInfo.decodeTag(tag, false));
		
		mStateStack.push(State.STATE_NORMAL);
		//referenceValue.visit(this, referenceValue, referenceValue);
		value.visit(this, rootTdf, value);
		mStateStack.pop();
		
		if (mStateStack.peek() == State.STATE_MAP) {
			// Tdf has been visited; the next field hit 
			// will be a map key.
			MapState mst = mMapStateStack.peek();
			mst.mNextIsKey = true;
		}

		mNestedTags.pop();
		
		mTagInfoMap = old;
		
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Tdf roottdf, Tdf parentTdf, long tag, Union value,
			Union referenceValue) {

		mActiveUnionMember = value.getActiveMember();
		if(mActiveUnionMember != Union.INVALID_MEMBER_INDEX) {
			TagInfoMap old = mTagInfoMap;
			mTagInfoMap = value.getTagInfoMap();
			
			mStateStack.push(State.STATE_UNION);
			mNestedTags.push(TagInfo.decodeTag(tag, false));
			value.visit(this, roottdf, referenceValue);
			mNestedTags.pop();
			mStateStack.pop();
			
			mTagInfoMap = old;
		}
		mActiveUnionMember = Union.INVALID_MEMBER_INDEX;
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.VariableTdfContainer, ea.blaze.tdf.VariableTdfContainer)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag,
		VariableTdfContainer value, VariableTdfContainer referenceValue) {
		
		// Encode the TDF_ID of the Tdf contained in the variable container.
		if (value.isValid()) {
			TdfLong tdfId = new TdfLong(value.get().getTdfId());
			visit(rootTdf, parentTdf, tag, tdfId, tdfId, 0);
			
			// Encode the Tdf contained in the variable container.
			mNestedTags.push(TagInfo.decodeTag(tag, false));
			value.get().visit(this, value.get(), value.get());
			mNestedTags.pop();
		}
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectType, ea.blaze.tdf.BlazeObjectType)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectType value, BlazeObjectType referenceValue) {
		
		addToRequestString(tag, value.toString());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectId, ea.blaze.tdf.BlazeObjectId)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectId value, BlazeObjectId referenceValue) {
		addToRequestString(tag, value.toString());

	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TimeValue value,
			TimeValue referenceValue, TimeValue defaultValue) {
		TdfLong val = new TdfLong(value.getMicroSeconds());
		visit(roottdf, parentTdf, tag, val, val, 0L);
	}
	
	/**
	 * Adds the string to the outgoing HTTP encoded parameters
	 * 
	 * @param s
	 *            the string to append
	 */
	private void addToRequestString(String s) {
		try {
			mParamBuffer.put(s);
		}
		catch (Exception ex) { 
			mResult = false;
			System.err.println("Failed to add HTTP parameter string to output buffer: " + ex.getMessage());
			ex.printStackTrace(System.err); 
		}
	}
	
	/**
	 * Adds the tag and string to request string.
	 * 
	 * @param tag
	 *            the tag
	 * @param s
	 *            the s
	 */
	private void addToRequestString(long tag, String s)
	{
		try {
			String parentTags = parentTagsStr();
			
			String thisTag = parentTags;

			
			if (mStateStack.peek() == State.STATE_ARRAY) {
				VectorState vs = mVectorStateStack.peek();

				// Handle an array of primitives.
				if (tag == Tdf.CONTAINER_MEMBER) {
					thisTag += "|" + vs.vectorIndex;
					vs.vectorIndex++;
				}
				mParamBuffer.put(thisTag);
				mParamBuffer.put("=");
				mParamBuffer.put(s);
				mParamBuffer.put("&");
			}
			else if (mStateStack.peek() == State.STATE_MAP){
				MapState mst = mMapStateStack.peek();
				
				if (mst.mNextIsKey) {
					// Key
					mNestedTags.push(s/*the key*/);
					mst.mNextIsKey = false;
				}
				else {
					// Value. This is for a map of primitives only. For a map of TDFs,
					// control will fall to "Normal Field" below.
					mParamBuffer.put(thisTag);
					mParamBuffer.put("=");
					mParamBuffer.put(s);
					mParamBuffer.put("&");
					mst.mNextIsKey = true;
					mNestedTags.pop();
				}
			}
			else if (mStateStack.peek() == State.STATE_UNION){
				// Use the member name, not the tag for unions.
				// The docs say to use the tag, but the comments
				// in the C++ HttpDecoder say to use the member
				// name.
				
				TagInfo ti = mTagInfoMap.getTagAtIndex(mActiveUnionMember);
				if (ti != null) {
					if (!thisTag.isEmpty())
						thisTag += "|";

					thisTag += ti.getMember();
					
					mParamBuffer.put(thisTag);
					mParamBuffer.put("=");
					mParamBuffer.put(s);
					mParamBuffer.put("&");
				}
			}
			else {
				// Normal Field
				if (!thisTag.isEmpty())
					thisTag += "|";
				thisTag += TagInfo.decodeTag(tag,false);
				
				mParamBuffer.put(thisTag);
				mParamBuffer.put("=");
				mParamBuffer.put(s);
				mParamBuffer.put("&");
			}
		}
		catch (Exception ex) { 
			mResult = false;
			System.err.println("Failed to add HTTP parameter string to OutputStream: " + ex.getMessage());
			ex.printStackTrace(System.err); 
		}
	}
	
	/**
	 * Get string containing nested parents parameters for nested TDF and container values.
	 * 
	 * @return the string
	 */
	private String parentTagsStr() {
		String parentTags = "";
		for (String t : mNestedTags) {
			if (!t.isEmpty())
				parentTags += t + "|";
		}
		
		// Chop off the trailing |
		if (!parentTags.isEmpty()) {
			parentTags = parentTags.substring(0,parentTags.length()-1);
		}
		
		return parentTags;
	}


	/* (non-Javadoc)
	 * @see ea.blaze.tdf.protocol.TdfEncoder#reset()
	 */
	@Override
	protected void reset() {
		mResult = false;
		mTagInfoMap = null;
		mStateStack.clear();
		mNestedTags.clear();
		mActiveUnionMember = Union.INVALID_MEMBER_INDEX;
		mMapStateStack.clear();
		mVectorStateStack.clear();	
	}
	
}
