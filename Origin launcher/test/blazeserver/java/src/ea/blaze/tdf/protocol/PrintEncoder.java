/**
 *  PrintEncoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.protocol;

import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintStream;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.util.TreeMap;
import java.util.Stack;
import java.util.Vector;

import ea.blaze.protocol.Encoder;
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
 * The Class PrintEncoder.
 */
public class PrintEncoder extends TdfEncoder
{
	
	/** The Constant INDENT_STRING. */
	private static final String INDENT_STRING = "   ";
	
	/**
	 * The Enum StateType.
	 */
	private enum StateType {
		 STATE_TDF,
		 STATE_ARRAY,
		 STATE_MAP,
		 STATE_UNION,
		 STATE_VARIABLE_TDF
	};

	/**
	 * The Class State.
	 */
	private class State {
		
		/** The type. */
		private StateType mType;
		
		/** The tag info map. */
		private TagInfoMap mTagInfoMap;
		
		// For array state
		/** The element num. */
		private int mElementNum = 0;
		
		/** The element count. */
		private int mElementCount = 0;
		
		// For container state
		/** The member name. */
		private String mMemberName = "";
		
		// For map state
		/** The is key. */
		private boolean mIsKey = true;
		
		/** The map key. */
		private String mMapKey = "";
		
		// For union state
		/** The active member. */
		private int mActiveMember = Union.INVALID_MEMBER_INDEX;
		
		/**
		 * Instantiates a new state.
		 * 
		 * @param stateType
		 *            the state type
		 * @param tagInfoMap
		 *            the tag info map
		 */
		public State(StateType stateType, TagInfoMap tagInfoMap) {
			mType = stateType;
			mTagInfoMap = tagInfoMap;
		}
		
		/**
		 * Gets the type.
		 * 
		 * @return the type
		 */
		public StateType getType() {
			return mType;
		}
		
		/**
		 * Gets the tag info map.
		 * 
		 * @return the tag info map
		 */
		public TagInfoMap getTagInfoMap() {
			return mTagInfoMap;
		}
		
		/**
		 * Gets the member name.
		 * 
		 * @return the member name
		 */
		public String getMemberName() { 
			return mMemberName;
		}
		
		/**
		 * Sets the member name.
		 * 
		 * @param memberName
		 *            the new member name
		 */
		public void setMemberName(String memberName) {
			mMemberName = memberName;
		}

		/**
		 * Gets the active member.
		 * 
		 * @return the active member
		 */
		public int getActiveMember() {
			return mActiveMember;
		}
		
		/**
		 * Sets the active member.
		 * 
		 * @param activeMember
		 *            the new active member
		 */
		public void setActiveMember(int activeMember) {
			mActiveMember = activeMember;
		}
		
		/**
		 * Sets the element count.
		 * 
		 * @param elemCount
		 *            the new element count
		 */
		public void setElementCount(int elemCount) {
			mElementCount = elemCount;
		}
		
		/**
		 * Gets the element count.
		 * 
		 * @return the element count
		 */
		public int getElementCount() {
			return mElementCount;
		}
		
		/**
		 * Gets the then inc element num.
		 * 
		 * @return the then inc element num
		 */
		public int getThenIncElementNum() { 
			int retVal = mElementNum;
			mElementNum++;
			return retVal;
		}
		
		/**
		 * Gets the checks if is map key.
		 * 
		 * @return the checks if is map key
		 */
		public boolean getIsMapKey() {
			return mIsKey;
		}
		
		/**
		 * Sets the map key.
		 * 
		 * @param key
		 *            the new map key
		 */
		public void setMapKey(String key) {
			mIsKey = !mIsKey;
			mMapKey = key;
		}
		
		/**
		 * Gets the map key.
		 * 
		 * @return the map key
		 */
		public String getMapKey() {
			mIsKey = !mIsKey;
			return mMapKey;
		}
	}

	/** The state stack. */
	private Stack<State> mStateStack = new Stack<State>();
	
	/** The print stream. */
	private PrintStream mPrintStream;
		
	/** The s print encoder. */
	private static PrintEncoder sPrintEncoder = new PrintEncoder();
	
	/**
	 * Prints the tdf.
	 * 
	 * @param stream
	 *            the stream
	 * @param tdf
	 *            the tdf
	 * @throws IOException 
	 */
	public static void printTdf(OutputStream stream, Tdf tdf) throws IOException {
		ByteBuffer encodedBuf = sPrintEncoder.encode(tdf);
		encodedBuf.flip();
		Channels.newChannel(stream).write(encodedBuf);
	}
	
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	public boolean visit(Tdf value, Tdf referenceValue) {
		
		mPrintStream = new PrintStream(getBufferAsOutputStream());
		
		String mObjectName = value.getClass().getSimpleName();
		
		if(mObjectName == null)
			return true;
	
		mPrintStream.println(mObjectName + " = {");
		
		mStateStack.push(new State(StateType.STATE_TDF, value.getTagInfoMap()));
		
		value.visit(this, value, value);
		
		mStateStack.pop();
		
		mPrintStream.println("}");
		
		return false;
	}
	
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, Tdf value, Tdf referenceValue) {
		
		printIndent();
		
		if(mStateStack.peek().getType() == StateType.STATE_ARRAY || mStateStack.peek().getType() == StateType.STATE_MAP) {
			if(!outputPreamble(tag, value.getClass().getSimpleName()))
				return false;
			printIndent();
		}
		else if(mStateStack.peek().getType() == StateType.STATE_VARIABLE_TDF) {
			mPrintStream.print(value.getClass().getSimpleName() + " = ");
		}
		else {	
			mPrintStream.print(memberName(tag) + " = ");
		}
		
		mPrintStream.println("{");
		
		mStateStack.push(new State(StateType.STATE_TDF, value.getTagInfoMap()));
		
		value.visit(this, rootTdf, value);
		
		mStateStack.pop();
		
		printIndent();
		
		mPrintStream.println("}");
		
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.Vector, java.util.Vector, ea.blaze.tdf.VectorHelper)
	 */
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Vector<?> value, Vector<?> referenceValue, VectorHelper memberVisitor) {
		String memberName = memberName(tag);
		printIndent();
		mPrintStream.println(memberName + " = {");
		mStateStack.push(new State(StateType.STATE_ARRAY, null));
		mStateStack.peek().setMemberName(memberName);
		mStateStack.peek().setElementCount(value.size());
		memberVisitor.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
		mStateStack.pop();
		printIndent();
		mPrintStream.println("}");
	}
	
	/**
	 * Output preamble.
	 * 
	 * @param tag
	 *            the tag
	 * @param type
	 *            the type
	 * @return true, if successful
	 */
	public boolean outputPreamble(long tag, String type) {	
		switch(mStateStack.peek().getType()) {
			case STATE_TDF:
				break;
			case STATE_UNION:
				String name = memberName(tag);
				mPrintStream.println(" = " + name);
				break;
			case STATE_ARRAY:
				mPrintStream.println(mStateStack.peek().getMemberName() + "[" + mStateStack.peek().getThenIncElementNum() + "] = ");
				break;
			case STATE_MAP:
				mPrintStream.println(mStateStack.peek().getMemberName() + "[" + mStateStack.peek().getMapKey() + "] = ");
				break;
		}
		
		return true;
	}
		
	/**
	 * Member name.
	 * 
	 * @param tag
	 *            the tag
	 * @return the string
	 */
	public String memberName(long tag) {
		
		switch(mStateStack.peek().getType()) {
		case STATE_TDF:

			TagInfo info = mStateStack.peek().getTagInfoMap().get(tag);
			
			if(info != null) {
				return info.getMember();
			}
			else {
				return "";
			}
		case STATE_UNION:
			TagInfo memInfo = null;
			
			if(mStateStack.peek().getActiveMember() != Union.INVALID_MEMBER_INDEX) {
				memInfo = mStateStack.peek().getTagInfoMap().getTagAtIndex(mStateStack.peek().getActiveMember());
			}
			
			if(memInfo != null) {
				return memInfo.getMember();
			}
			else {
				return "";
			}
		case STATE_ARRAY:
			return mStateStack.peek().getMemberName() + "[" + mStateStack.peek().getThenIncElementNum() + "]";
		case STATE_MAP:
			return mStateStack.peek().getMemberName() + "[" + mStateStack.peek().getMapKey() + "]";
		default:
			return "";
		}
	}
	
	/**
	 * Output postamble.
	 * 
	 * @return true, if successful
	 */
	public boolean outputPostamble() {
		
		switch(mStateStack.peek().getType()) {
		
		case STATE_TDF:
			break;
		case STATE_UNION:
			mPrintStream.println();
			break;
		case STATE_ARRAY:
			mPrintStream.println();
			break;
		case STATE_MAP:
			int elementNum = mStateStack.peek().getThenIncElementNum();
			if(elementNum == 0) {
				mPrintStream.print(", ");
			}
			else if(elementNum == mStateStack.peek().getElementCount()-1) {
				mPrintStream.println(")");
			}
			break;
		}
		
		return true;
	}

	/**
	 * Gets the current state.
	 * 
	 * @return the current state
	 */
	public StateType getCurrentState() {
		return mStateStack.peek().getType();
	}
	
	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	public boolean visit(Union tdf, Union referenceValue) {
		mStateStack.push(new State(StateType.STATE_UNION, tdf.getTagInfoMap()));
		tdf.visit(this, tdf, referenceValue);
		mStateStack.pop();
		return true;
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.TreeMap, java.util.TreeMap, ea.blaze.tdf.MapHelper)
	 */
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TreeMap<?,?> value,
			TreeMap<?,?> referenceValue, MapHelper mapVisitor) {
		String memberName = memberName(tag);
		printIndent();
		mPrintStream.println(memberName + " = {");
		mStateStack.push(new State(StateType.STATE_MAP, null));
		mStateStack.peek().setMemberName(memberName);
		mStateStack.peek().setElementCount(value.size());
		mapVisitor.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
		mStateStack.pop();
		printIndent();
		mPrintStream.println("}");
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfBoolean, ea.blaze.tdf.types.TdfBoolean, boolean)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfBoolean value,
			TdfBoolean referenceValue, boolean defaultValue) {
		
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value);
	}
    
    /* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfChar, ea.blaze.tdf.types.TdfChar, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfChar value,
			TdfChar referenceValue, char defaultValue) {

            if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value);
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfByte, ea.blaze.tdf.types.TdfByte, byte)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfByte value,
			TdfByte referenceValue, byte defaultValue) {
		
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value);
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfShort, ea.blaze.tdf.types.TdfShort, short)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfShort value,
			TdfShort referenceValue, short defaultValue) {
		
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value);
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, int)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, int defaultValue) {
		
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value);
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfLong, ea.blaze.tdf.types.TdfLong, long)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfLong value,
			TdfLong referenceValue, long defaultValue) {
		
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value + "L");
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfUInt64 value,
			TdfUInt64 referenceValue, TdfUInt64 defaultValue) {
		
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value.toString());
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfFloat, ea.blaze.tdf.types.TdfFloat, float)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfFloat value,
			TdfFloat referenceValue, float defaultValue) {
		
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value);
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Bitfield, ea.blaze.tdf.Bitfield)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Bitfield value,
			Bitfield referenceValue) {
		
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(value.toString());
				return;
			}
		}
		
		printIndent();
		mPrintStream.println(memberName(tag) + " bitfield bit here hopefully -> " + value.getBits());
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfString, ea.blaze.tdf.types.TdfString, java.lang.String, long)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfString value,
			TdfString referenceValue, String defaultValue, long maxLength) {
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey("\"" + value.get() + "\"");
				return;
			}
		}
		printIndent();
		mPrintStream.println(memberName(tag) + " = \"" + value + "\"");
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Blob, ea.blaze.tdf.Blob)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Blob value,
			Blob referenceValue) {
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + new String(value.getData()));
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, java.lang.Class, java.lang.Enum)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, TdfEnum enumHelper, TdfEnum defaultValue) {
		if(getCurrentState() == StateType.STATE_MAP) {
			if(mStateStack.peek().getIsMapKey()) {
				mStateStack.peek().setMapKey(enumHelper.lookupValue(value.get()).getName());
				return;
			}
		}
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + enumHelper.lookupValue(value.get()).getName() + "[" + value.get().toString() + "]");
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	public boolean visit(Tdf roottdf, Tdf parentTdf, long tag, Union value,
			Union referenceValue) {
		printIndent();
		mPrintStream.println(memberName(tag) + " = {");
		mStateStack.push(new State(StateType.STATE_UNION, value.getTagInfoMap()));
		mStateStack.peek().setActiveMember(value.getActiveMember());
		value.visit(this, value, referenceValue);
		mStateStack.pop();
		printIndent();
		mPrintStream.println("}");
		return false;
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.VariableTdfContainer, ea.blaze.tdf.VariableTdfContainer)
	 */
	public boolean visit(Tdf roottdf, Tdf parentTdf, long tag,
			VariableTdfContainer value, VariableTdfContainer referenceValue) {
		
		boolean result = true;
		
		if(value.isValid()) {
			printIndent();
			mPrintStream.println(memberName(tag) + " = {");
			mStateStack.push(new State(StateType.STATE_VARIABLE_TDF, value.get().getTagInfoMap()));
			result = visit(roottdf, parentTdf, tag, value.get(), referenceValue.get());
			mStateStack.pop();
			printIndent();
			mPrintStream.println("}");
		}
		
		return result;
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectType, ea.blaze.tdf.BlazeObjectType)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectType value, BlazeObjectType referenceValue) {
		printIndent();
		mPrintStream.println(memberName(tag) + " = (BlazeObjectType) " + value.toString());
		
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectId, ea.blaze.tdf.BlazeObjectId)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectId value, BlazeObjectId referenceValue) {
		printIndent();
		mPrintStream.println(memberName(tag) + " = (BlazeObjectId) " + value.toString());
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue)
	 */
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TimeValue value,
			TimeValue referenceValue, TimeValue defaultValue) {
		printIndent();
		mPrintStream.println(memberName(tag) + " = " + value.toString());
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.protocol.Encoder#getName()
	 */
	public String getName() {
		return "print";
	}

	//@override
	/* (non-Javadoc)
	 * @see ea.blaze.protocol.Encoder#getType()
	 */
	public Type getType() {
		return Encoder.Type.PRINT;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.protocol.TdfEncoder#reset()
	 */
	@Override
	protected void reset() {
		mStateStack.clear();
	}
	
	/**
	 * Prints the indent.
	 */
	private void printIndent() {
		for(int i = 0; i < mStateStack.size(); i++) {
			mPrintStream.print(INDENT_STRING);
		}
	}
}
