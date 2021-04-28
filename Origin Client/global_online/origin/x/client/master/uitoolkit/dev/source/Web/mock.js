$(document).ready(function() {
	var inputGroup = $('#SingleLineEdit');
	var inputBox = new origin.toolkit.SingleLineEdit({
		enabled: true,
		maxLength: 200,
		password: false,
		placeHolderText: "Enter Username"
	});
	//inputBox.setText("Click Me!");
	inputGroup.append(inputBox.element);
	
	var inputBox1 = new origin.toolkit.SingleLineEdit({
		enabled: false
	});
	inputBox1.setText("EARS");	
	inputGroup.append(inputBox1.element);

	var inputBox2 = new origin.toolkit.SingleLineEdit({
		enabled: true,
		password: true
	});
	inputBox2.setText("password");
	inputGroup.append(inputBox2.element);
		
	var inputBox3 = new origin.toolkit.SingleLineEdit({
		enabled: true,
		error: true
	});
	inputBox3.setText("Error");
	inputGroup.append(inputBox3.element);
	var inputBox4 = new origin.toolkit.SingleLineEdit({
		enabled: true,
		readOnly: true
	});
	inputBox4.setText("Read Only");
	inputGroup.append(inputBox4.element);
	
	var inputBox5 = new origin.toolkit.SingleLineEdit({
		enabled: true,
		readOnly: false,
	});
	inputBox5.setText("Valid");
	inputBox5.setValidationStatus("Valid");
	inputGroup.append(inputBox5.element);
	
	var inputBox6 = new origin.toolkit.SingleLineEdit({
		enabled: true,
		maxLength: 100,
		password: false,
		placeHolderText: "Enter Username",
		inputAccessory: "Search"
	});
	inputGroup.append(inputBox6.element);
	
	inputBox6.inputAccessory.add(function(e){
		alert(inputBox.getText());
	});
	
	var buttonGroup = $('#PushButton');
	
	var button1 = new origin.toolkit.PushButton({
		enabled: true,
		text: "Primary", 
		defaultButton: false, 
		buttonType: "Primary", 
	});
	buttonGroup.append(button1.element);
	
	var button2 = new origin.toolkit.PushButton({
		enabled: true,
		text: "Secondary", 
		defaultButton: false, 
		buttonType: "Secondary",
	});
	buttonGroup.append(button2.element);

	var button3 = new origin.toolkit.PushButton({
		enabled: true,
		text: "Tertiary",
		defaultButton: false,
		buttonType: "Tertiary",
		id: "button3",
	});
	button3.clickEvent.add(function(e){
		alert("Tertiary is Default");
	});
	buttonGroup.append(button3.element);
	
	buttonGroup.append("<br/><br/>");
	
	var button4 = new origin.toolkit.PushButton({
		enabled: false,
		text: "Dis Primary",
		defaultButton: false,
		buttonType: "Primary",
	});
	buttonGroup.append(button4.element);

	var button5 = new origin.toolkit.PushButton({
		enabled: false,
		text: "Dis Secondary",
		defaultButton: false,
		buttonType: "Secondary",
	});
	buttonGroup.append(button5.element);

	var button6 = new origin.toolkit.PushButton({
		enabled: false,
		text: "Dis Tertiary",
		defaultButton: false,
		buttonType: "Tertiary",
	});
	buttonGroup.append(button6.element);
	
	// Some testing methods
	//button4.setEnabled(true);
	//button4.setText("Test Text Change");
	//button4.setButtonType("Primary");
	button3.setFocus();
	button3.setDefaultButton();
	var radioGroup = $('#RadioButton');
	
	var radio1 = new origin.toolkit.RadioButton({
		enabled: true,
		id: "test1",
		name: "radios",
		text: "Default"
	});
	radioGroup.append(radio1.element);

	radioGroup.append("<br>");
	var radio2 = new origin.toolkit.RadioButton({
		enabled: true,
		id: "test2",
		name: "radios",
		text: "Focus"
	});
	radioGroup.append(radio2.element);
	radio2.setFocus();	
	
	radioGroup.append("<br>");
	var radio3 = new origin.toolkit.RadioButton({
		enabled: true,
		id: "test3",
		name: "radios",
		text: "Active"
	});
	radioGroup.append(radio3.element);
	radio3.setChecked(true);

	radioGroup.append("<br>");
	var radio4 = new origin.toolkit.RadioButton({
		enabled: true,
		id: "test4",
		name: "radios",
		text: "Disabled",
	});
	radioGroup.append(radio4.element);
	radio4.setEnabled(false);
	
	radioGroup.append("<br>");
	var radio5 = new origin.toolkit.RadioButton({
		enabled: true,
		checked: true,
		id: "test5",
		name: "radio2",
		text: "Disabled Checked",
	});
	radioGroup.append(radio5.element);
	radio5.setEnabled(false);
	
	radioGroup.append("<br>");
	var radio6 = new origin.toolkit.RadioButton({
		enabled: true,
		checked: false,
		id: "test6",
		name: "radio3",
		text: "Invalid"
	});
	radioGroup.append(radio6.element);
	radio6.setValidationStatus("Invalid");	
	var bannerGroup = $('#Banner');
	
	var banner1 = new origin.toolkit.Banner({
		text: "You've successfully activated the following product(s).",
		bannerType: "Banner"
	});
	bannerGroup.append(banner1.element);
	
	var banner2 = new origin.toolkit.Banner({
		text: "There’s already an account with this email.",
		bannerType : "Banner",
		iconType : "Info"
	});
	bannerGroup.append(banner2.element);
	
	var banner3 = new origin.toolkit.Banner({
		text: "Could not process your request.",
		bannerType : "Banner",
		iconType : "Error"
	});
	bannerGroup.append(banner3.element);
	
	var banner4 = new origin.toolkit.Banner({
		text: "Title Header",
		bannerType: "Banner",
		iconType : "Notice"
	});
	bannerGroup.append(banner4.element);
	
	var banner5 = new origin.toolkit.Banner({
		text: "Password Successfully Changed",
		bannerType: "Message",
		iconType : "Success"
	});
	$('#special').append(banner5.element);
	
	var inputGroup = $('#CheckButton')
	
	var checkBox = new origin.toolkit.CheckButton({
		enabled: true,
		text: "Unchecked",
		id: "checkBox"
	});
	
	inputGroup.append(checkBox.element);
	//checkBox.setFocus();
	//checkBox.setValidationStatus("Invalid");
	inputGroup.append('<br>');
	
	var checkBox1 = new origin.toolkit.CheckButton({
		id: "checkBox1",
		text: "Checked",
		enabled: true,
		checked: true
	});
	inputGroup.append(checkBox1.element);
	
	inputGroup.append('<br>');	
	
	var checkBox2 = new origin.toolkit.CheckButton({
		id: "checkBox2",
		text: "Disabled",
		enabled: false
	});
	inputGroup.append(checkBox2.element);
	
	inputGroup.append('<br>');
	var checkBox3 = new origin.toolkit.CheckButton({
		id: "checkBox3",
		text: "Disabled Checked",
		enabled: false,
		checked: true		
	});
	inputGroup.append(checkBox3.element);
	checkBox3.setFocus();
	
	inputGroup.append('<br>');
	var checkBox4 = new origin.toolkit.CheckButton({
		id: "checkBox4",
		text: "Error Unchecked",
		enabled: true,
		validationStatus: "Invalid"
	});
	inputGroup.append(checkBox4.element);
	
	inputGroup.append('<br>');
	var checkBox5 = new origin.toolkit.CheckButton({
		id: "checkBox5",
		text: "Error Checked",
		enabled: true,
		checked: true,
		validationStatus: "Invalid"
	});
	inputGroup.append(checkBox5.element);

	var tipGroup = $('#InfoTip');
	var infoTip1 = new origin.toolkit.InfoTip({
		text: "s",
		tip: "Lorem ipsum dolor sit amet consectetur.",
		size: "small",
	});
	tipGroup.append(infoTip1.element);
	tipGroup.append("<br><br>");
	var infoTip2 = new origin.toolkit.InfoTip({
		text: "m",
		tip: "Lorem ipsum dolor sit amet consectetur adipin sicing elit sed do eiusmod tempor incididunt labore.",
		size: "medium"
	});
	tipGroup.append(infoTip2.element);
	tipGroup.append("<br><br>");
	var infoTip3 = new origin.toolkit.InfoTip({
		text: "l",
		tip: "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim venim, quis nostrud exercitation.",
		size: "large"
	});
	tipGroup.append(infoTip3.element);		
});

