/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: samqfsui.js,v 1.25 2008/05/16 19:39:12 am143972 Exp $


   // trim (STRING myString)
   // Usage: Remove leading and trailing spaces from the passed string.
   //        If something besides a string is passed in, such as unknown
   //        objects or null String, function will return the input.

   function trim(myString) {
      if (typeof myString != "string")
         return myString;

      var retValue = myString;
      var c = retValue.substring(0, 1);

      // start removing leading spaces
      while (c == " ") {
         retValue = retValue.substring(1, retValue.length);
         c = retValue.substring(0, 1);
      }

      c = retValue.substring(retValue.length-1, retValue.length);

      // start removing trailing spaces
      while (c == " ") {
         retValue = retValue.substring(0, retValue.length-1);
         c = retValue.substring(retValue.length-1, retValue.length);
      }

      // return the trimmed string
      return retValue; 
   }

   // isValidNum (STRING numStr, STRING numUnit,
   //			STRING startStr, STRING startUnit,
   //			STRING endStr, STRING endUnit)
   // return: -1 (Developer bug)
   //          0 (not a valid number, or, not in the range)
   //          1 (valid number)
   function isValidNum(numStr, numUnit, startStr, startUnit,
							endStr, endUnit) {

      // see return value definition in checkSyntax()
      var usage = checkSyntax(numStr, numUnit, startStr, startUnit,
							endStr, endUnit);

      if (usage == -1) return usage;
      else if (usage == 1) {
	 if (isInteger(numStr)) return 1;
	 else return 0;

      } else if (usage == 2) {
	 if (!isInteger(numStr) || !isInteger(startStr) || !isInteger(endStr))
	    return 0;
         else
	    return ((parseInt(numStr) >= parseInt(startStr)) &&
		    (parseInt(numStr) <= parseInt(endStr)));

      } else if (usage == 3) {
	 if (!isInteger(numStr) || !isInteger(startStr) || !isInteger(endStr))
            return 0;
	 else {
	    if (isNumInRange(numStr, numUnit, startStr, startUnit,
					endStr, endUnit)) return 1;
	    else return 0;
	 }
      } else {
         alert("Shouldn't come here");
      }
   }

   // checkSyntax(STRING, STRING, STRING, STRING, STRING, STRING)
   // return: -1 (Error situation)
   //          1 (going to check numStr only)
   //          2 (going to check if numStr is in range without UNIT)
   //          3 (going to check if numStr is in range WITH UNIT) 
   function checkSyntax(numStr, numUnit, startStr, startUnit, endStr, endUnit) {

       // if main value that needs to be checked is empty, return error
       if (isEmpty(numStr)) return -1;

       // make sure if start value is defined, so as the end value, or vice
       // versa
       if (isEmpty(startStr) && !isEmpty(endStr)) return -1;
       if (!isEmpty(startStr) && isEmpty(endStr)) return -1;

       // make sure if start unit is defined, so as the startStr
       // same as endStr & endUnit
       // Note: It's valid that user only defines the str but not the unit
       if (isEmpty(startStr) && !isEmpty(startUnit)) return -1;
       if (isEmpty(endStr) && !isEmpty(endUnit)) return -1;

       // make sure if numUnit is defined, so as the other two units
       if (! ((isEmpty(numUnit) && isEmpty(startUnit) && isEmpty(endUnit)) ||
	     (!isEmpty(numUnit) && !isEmpty(startUnit) && !isEmpty(endUnit))))
          return -1;

       // make sure all the units are in the same set, else error
       if (detectType(numUnit, startUnit, endUnit) == "-1") return -1;

       if (isEmpty(numUnit) && isEmpty(startStr) && isEmpty(startUnit) &&
           isEmpty(endUnit) && isEmpty(endStr)) return 1;
       else if (isEmpty(startUnit) && isEmpty(endUnit)) return 2;
       else return 3;
   }

   // return: -1 (types are not in same set)
   //          1 (no type)
   //          2 (time type)
   //          3 (size type)
   function detectType(numUnit, startUnit, endUnit) {

      if (isEmpty(numUnit) && isEmpty(startUnit) && isEmpty(endUnit))
         return 1;

      var isTimeType, startMatch, endMatch;
      var timeArr = new Array(5);
      var sizeArr = new Array(5);

      timeArr[0] = "sec";
      timeArr[1] = "min";
      timeArr[2] = "hr";
      timeArr[3] = "day";
      timeArr[4] = "wk";

      sizeArr[0] = "b";
      sizeArr[1] = "kb";
      sizeArr[2] = "mb";
      sizeArr[3] = "gb";
      sizeArr[4] = "tb";
      sizeArr[5] = "pb";

      // check to see what numUnit belongs to
      if (numUnit == "sec" || numUnit == "min" || numUnit == "hr" ||
          numUnit == "day" || numUnit == "wk")
         isTimeType = true;
      else if (numUnit == "b" || numUnit == "kb" || numUnit == "mb" ||
          numUnit == "gb" || numUnit == "tb" || numUnit == "pb")
         isTimeType = false;
      else 
         return -1; // wrong type, neither time nor size

      // numUnit is a type of time, check units of other two
      if (isTimeType) {
	 for (var i = 0; i < timeArr.length; i++) {
	    if (startUnit == timeArr[i]) startMatch = true;
	    if (endUnit   == timeArr[i]) endMatch = true;
	 }
      } else {
	 // numUnit is a type of size, check units of other two
         for (var i = 0; i < sizeArr.length; i++) { 
            if (startUnit == sizeArr[i]) startMatch = true;
            if (endUnit   == sizeArr[i]) endMatch = true;
         }
      }

      if (startMatch && endMatch) {
	 if (isTimeType) return 2;
	 else return 3;
      } else return -1;
   } 

   function isNumInRange(numStr, numUnit, startStr, startUnit,
            		                            endStr, endUnit) {
      var type = detectType(numUnit, startUnit, endUnit);

      // type can only be "2" or "3" 
      // "2" ==> time
      // "3" ==> size

      if (type == "2") {
	 var num   = convertToSec(numStr, numUnit);
	 var start = convertToSec(startStr, startUnit);
	 var end   = convertToSec(endStr, endUnit);

	 return ( (num >= start) && (num <= end) );

      } else if (type == "3") {
         var num   = convertToByte(numStr, numUnit);
         var start = convertToByte(startStr, startUnit);
         var end   = convertToByte(endStr, endUnit);

         return ( (num >= start) && (num <= end) );

      } else {
         alert("Shouldn't come here");
	 return false;
      }
   }

   function convertToSec(numStr, numUnit) {

      var retValue = 0;
      var baseValue = 1;

      if (numUnit == "sec") baseValue = 1;
      else if (numUnit == "min") baseValue = 60;
      else if (numUnit == "hr") baseValue = 60 * 60;
      else if (numUnit == "day") baseValue = 60 * 60 * 24;
      else if (numUnit == "wk") baseValue = 60 * 60 * 24 * 7;

      return (parseInt(numStr) * baseValue);
   }

   function convertToByte(numStr, numUnit) {

      var retValue = 0;
      var baseValue = 1;

      if (numUnit == "b") baseValue = 1;
      else if (numUnit == "kb") baseValue = 1024;
      else if (numUnit == "mb") baseValue = 1024 * 1024;
      else if (numUnit == "gb") baseValue = 1024 * 1024 * 1024;
      else if (numUnit == "tb") baseValue = 1024 * 1024 * 1024 * 1024;
      else if (numUnit == "pb") baseValue = 1024 * 1024 * 1024 * 1024 * 1024;

      return (parseInt(numStr) * baseValue);
   }


   // isValidString (STRING myString, STRING[] specArr)
   function isValidString(myString, specArr) {

      // if myString is empty, or contain spaces in between content,
      // return false
      if (doesCharExist(myString, " "))
	 return false;

      // now if specArr contains something, check to see if myString
      // contains any of the characters

      if (specArr.length == 0) return true; // no need to check special char
      else {
         if (specArr == "alphaNum") {
	    // check if myString is a alpha-numeric string
	    for (var i = 0; i < myString.length; i++) {
	       var c = myString.charAt(i);
	       if (!isAlphaNumeric(c)) return false;
	    }
	    return true;

	 } else {
	    // check if myString contains any of the special characters that
            // are defined in specArr
	    for (var i = 0; i < specArr.length; i++) {
	       if (doesCharExist(myString, specArr[i])) return false;
	    }
	    return true;
	 }
      } // end if (specArr.length == 0)
   } // end function isValidString()

   // isValidIP(STRING myString)
   // Usage: Check if myString is a valid IP Address

   function isValidIP(myString) {

      if (doesCharExist(myString, " "))
         return false;

      // now, check to see if myString is in XXX.XXX.XXX.XXX format
      var myIPArray = myString.split(".");

      if (myIPArray.length != 4) return false;
      else {
	 // check each element in myIPArray, make sure they are positive integer
	 // (include 0) and in the range of 0 to 255
	 for (var i = 0; i < myIPArray.length; i++) {
	    if (!isInteger(myIPArray[i])) return false;
	    else {
	       var number = parseInt(myIPArray[i]);
	       if (number < 0 || number > 255) return false;
	    }
	 }
	 return true;
      }
   }

   function isValidLogFile(str) {
	var invalidArray = new Array(21);
	invalidArray[0] = "?";
	invalidArray[1] = "*";
	invalidArray[2] = "\\";
	invalidArray[3] = "!";	
	invalidArray[4] = "&";
	invalidArray[5] = "%";
	invalidArray[6] = "'";
	invalidArray[7] = "\"";
	invalidArray[8] = "(";
	invalidArray[9] = ")";
	invalidArray[10] = "+";
	invalidArray[11] = ",";
	invalidArray[12] = ":";
	invalidArray[13] = ";";
	invalidArray[14] = "<";
	invalidArray[15] = ">";
	invalidArray[16] = "@";
	invalidArray[17] = "#";
	invalidArray[18] = "$";
	invalidArray[19] = "=";
	invalidArray[20] = "^";

	if (!isValidString(str, invalidArray)) 
                return false;

	return true;
   }

   function doesCharExist(myString, specChar) {

      var myArray = myString.split(specChar);
      if (myArray.length != 1)
         return true;
      else
         return false;
   }

   function isInteger(s) {
      if (doesCharExist(s, " "))
         return false;

      for (var i = 0; i < s.length; i++) {
         var c = s.charAt(i);
         if (!isDigit(c)) return false;
      }

      return true;
   }

   function isEmpty(str) {
      return ((str == null) || (str.length == 0));
   }

   function isDigit(c) {
      return ((c >= "0") && (c <= "9"));
   }

   function isAlphaNumeric(c) {
      return (isDigit(c) || ((c >= "a") && (c <= "z")) ||
        ((c >= "A") && (c <= "Z")));
   }

   function isValidFileNameCharacter(c) {
       return (isDigit(c) || ((c >= "a") && (c <= "z")) ||
               ((c >= "A") && (c <= "Z")) || (c == "-") || (c == "_"));
   }

   function isHexDigit(c) {
      return (isDigit(c) || 
         ((c >= "a") && (c <= "f")) || ((c >= "A") && (c <= "F")));
   }

   function isValidVSNRangeString(rangeValue) {
 
      if (!isValidString(rangeValue, "")) return false;

      var splitComma = rangeValue.split(",");

      // now put into loop to parse for dash
      for (var i = 0; i < splitComma.length; i++) {
          var splitDash = splitComma[i].split("-");

          // now check on each element in splitComma to see if
          // they are in valid VSN Format
          for (var j = 0; j < splitDash.length; j++)
             if (!isValidVSNString(splitDash[j])) return false;
      }
      return true;
   }

   function isValidVSNString(s) {
      if (doesCharExist(s, " "))
         return false;

      // VSN name has to have a length smaller or equal to 6
      // if length of s is greater than 6, return false
      if (s.length > 6) return false;

      for (var i = 0; i < s.length; i++) {
         var c = s.charAt(i);
         if (!isVSNDigit(c)) return false;
      }

      return true;
   }

   function isVSNDigit(c) {
      return (isDigit(c) ||
	 ((c >= "A") && (c <= "Z")) || (c == "!") ||
	 (c == "\"") || (c == "%") || (c == "&") ||
	 (c == "'") || (c == "(") || (c == ")") ||
	 (c == "*") || (c == "+") || (c == ",") ||
	 (c == "-") || (c == ".") || (c == "/") ||
	 (c == ":") || (c == ";") || (c == "<") ||
	 (c == "=") || (c == ">") || (c == "?") || (c == "_"));
   }

   function isValidEmail(str) {
      if (!isValidEMailAdd(str)) return false;

      var index = str.indexOf("@");

      // Check if email address string contains "@"
      // If yes, do further validation
      // If no, simply return true because it is valid for the user
      // inputs only the username for email address.
      if (index > 0) {
	 // check for "." on right hand side of "@"
         var dotIndex = str.indexOf(".", index);

	 // it is valid to have no DOT in the address, such as
	 // "username@domain"
	 if (dotIndex == -1) {
	    // check to make sure "@" is not the end of the string
	    if (index+1 == str.length) return false;
	    else return true;

	 // make sure there is something in between the "@" and "."
	 // and there is something behind the "."
         } else if ((dotIndex > index+1) && (str.length > dotIndex+1))
	    return true;

	 else return false;
      } else return true;
   }

   function isPositiveInteger(s) {
      // s must be an integer, AND s is greater or equal to 0
      return (isInteger(s) && (parseInt(s) >= 0));
   }

   function isPositiveNonZeroInteger(s) {
      // s must be an integer, AND s is greater than 0
      return (isInteger(s) && (parseInt(s) > 0));
   }

   function isZero(s) {
      // s must be an integer, AND s is equal to 0
      return (isInteger(s) && (parseInt(s) == 0));
   }

   function getParentViewName(childName) {
        // Lockhart naming convention is "view1.view2...viewn.childname"
        // We want to lop off the child name and return the rest.
        var dotIndex = childName.lastIndexOf('.');
        var parentName = childName.substring(0, dotIndex);
        return parentName;
   }

    function getRowIndexFromName(childName) {
        // Lockhart naming convention for children in table is 
        // "view1.view2...tiledView[x].childname" where x is the row index.
        // This function grabs the x.  Kludgy ain't it?

        var openBracketIdx = childName.indexOf("[");
        var closeBracketIdx = childName.indexOf("]");
        return childName.substring(openBracketIdx + 1, closeBracketIdx);
    }

    function getSelectedRadioButton(formObj, buttonName) {
        var buttonArrayObj = formObj.elements[buttonName];
        if (buttonArrayObj == null) {
            return null;
        }
        // buttonArrayObj is not really an array if there is only one radio
        // button.  I HATE that.
        if (buttonArrayObj.checked) {
            // Only 1 so return it.
            return buttonArrayObj;
        } else {
            // It's an array
            for(i = 0; i < buttonArrayObj.length; i++) {
                if (buttonArrayObj[i].checked == true) {
                    return buttonArrayObj[i];
                }
            }
            return null;
        }
    }

    function makeURLSafe(value) {
        // First escape everything except the first 69 ASCII characters
        var escValue = escape(value);

        // According to the documentation, the escape function does not escape 
        // these characters:        @*-_+./
        // Of these, @ + / have special meaning in urls, so we need to escape 
        // those specially
        // Can't use the replace method because of differences if supportability    
        // across browser types.
        var searchChars = new Array("@", "+", "/");
        var replaceCodes = new Array("%40", "%2B", "%2F");
        
        for (i = 0; i < searchChars.length; i++) {
            var startIdx = 0;
            var endIdx = 0;
            var newValue = "";
            do {
                endIdx = escValue.indexOf(searchChars[i], startIdx);
                if (endIdx != -1) {
                    newValue = newValue + 
                               escValue.substring(startIdx, endIdx) + 
                               replaceCodes[i];
                    startIdx = endIdx + 1;
                }
            } while (endIdx != -1);

            // If any replace chars were found, add the end of the string
            // and stick it into escValue
            if (startIdx > 0) {
                escValue = newValue + escValue.substring(startIdx, escValue.length);
            }

        }

        return escValue;
        
    }
   function isValidEMailAdd(str) {
	var invalidArray = new Array(20);
	invalidArray[0] = "?";
	invalidArray[1] = "*";
	invalidArray[2] = "\\";
	invalidArray[3] = "!";	
	invalidArray[4] = "&";
	invalidArray[5] = "%";
	invalidArray[6] = "'";
	invalidArray[7] = "\"";
	invalidArray[8] = "(";
	invalidArray[9] = ")";
	invalidArray[10] = "+";
	invalidArray[11] = ",";
	invalidArray[12] = ":";
	invalidArray[13] = ";";
	invalidArray[14] = "<";
	invalidArray[15] = ">";
	invalidArray[16] = "#";
	invalidArray[17] = "$";
	invalidArray[18] = "=";
	invalidArray[19] = "^";

	if (!isValidString(str, invalidArray)) 
                return false;

	return true;
   }

    // Adds code to form action which causes the new page to scroll to the 
    // indicated anchor.  Works well with property sheets.
    // Code is largely like onClick code generated by HRef tags.
    function onClickWithAnchorJump(clickedObj, anchorName) {
        var f = clickedObj.form;
        if (f == null) {
            // Some objects, like links, don't have a form property.
            // Assume the page has only 1 form which should always be the case...
            // If it's not, this may break for the second form
            f = document.forms[0];
        }
        if (f != null) {
            f.action = f.action + "?" + clickedObj.name + "=" + clickedObj.value + "#" + anchorName;
            f.submit();
            return false;
        }
    }

    function getDropDownSelectedItem(dropdown) {
        var item = -1;
        if (dropdown != null) {
            item = dropdown.value;
        }
        return item;
    }
    
    function resetDropDownMenu(dropdown) {
        if (dropdown != null) {
            dropdown.options[0].selected = true;
        }
    }
    

