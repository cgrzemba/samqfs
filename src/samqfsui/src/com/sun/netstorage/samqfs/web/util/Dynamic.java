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
 * or https://illumos.org/license/CDDL.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: Dynamic.java,v 1.2 2008/12/16 00:12:26 am143972 Exp $

// This class can be used to help generate a table dynamically.  It is not
// currently used by any classes yet.

package com.sun.netstorage.samqfs.web.util;

import com.sun.web.ui.component.Checkbox;
import com.sun.web.ui.component.Hyperlink;
import com.sun.web.ui.component.RadioButton;
import com.sun.web.ui.component.StaticText;
import com.sun.web.ui.component.Table;
import com.sun.web.ui.component.TableColumn;
import com.sun.web.ui.component.TableRowGroup;

import javax.faces.context.FacesContext;
import javax.faces.component.UIComponent;
import javax.faces.component.UIParameter;

public class Dynamic {
    public static final String CHECKBOX_ID = "select";
    public static final String HYPERLINK_ID = "link";

    // table id
    protected String tableId = null;

    // determine if table is single or multi select
    protected boolean singleSelect = true;

    // Default constructor.
    public Dynamic(String id) {
        this.tableId = id;
    }

    public Dynamic(String id, boolean singleSelect) {
        this.tableId = id;
        this.singleSelect = singleSelect;
    }

    // Note: When using tags in a JSP page, UIComponentTag automatically creates
    // a unique id for the component. However, when dynamically creating
    // components, via a backing bean, the id has not been set. In this
    // scenario, allowing JSF to create unique Ids may cause problems with
    // Javascript and components may not be able to maintain state properly.
    // For example, if a component was assigned "_id6" as an id, that means
    // there were 5 other components that also have auto-generated ids. Let us
    // assume one of those components was a complex component that, as part of
    // its processing, adds an additional non-id'd child before redisplaying the
    // view. Now, the id of this component will be "_id7" instead of "_id6".
    // Assigning your own id ensures that conflicts do not occur.

    // Get Table component.
    //
    // @param id The component id.
    // @param title The table title text.
    public Table getTable(String id, String title) {
        // Get table.
        Table table = new Table();

        table.setId(tableId);

        if (!singleSelect) {
            // Show deselect multiple button.
            table.setDeselectMultipleButton(true);
            // Show select multiple button.
            table.setSelectMultipleButton(true);
        }
        // Set title text.
        table.setTitle(title);

        return table;
    }

    // Get TableRowGroup component with header.
    //
    // @param id The component id.
    // @param sourceVar Value binding expression for model data variable.
    // @param sourceData Value binding expression for model data.
    // @param selected Value binding expression for selected property.
    // @param header Value binding expression for row group header text.
    public TableRowGroup getTableRowGroup(
        String id,
        String sourceVar,
        String sourceData,
        String selected, String header) {
        // Get table row group.
        TableRowGroup rowGroup = new TableRowGroup();
        // Set id.
        rowGroup.setId(id);
        // Set source var.
        rowGroup.setSourceVar(sourceVar);
        // Set header text.
        rowGroup.setHeaderText(header);
        // Set row highlight.
        setValueBinding(rowGroup, "selected", selected);
        // Set source data.
        setValueBinding(rowGroup, "sourceData", sourceData);

        return rowGroup;
    }

    // Get TableColumn component.
    //
    // @param id The component id.
    // @param sort Value binding expression for column sort.
    // @param align The field key for column alignment.
    // @param header The column header text.
    // @param selectId The component id used to select table rows.
    public TableColumn getTableColumn(
        String id,
        String sort,
        String align,
        String header, String selectId) {
        // Get table column.
        TableColumn col = new TableColumn();

        col.setId(id);
        col.setSelectId(selectId);
        col.setHeaderText(header);
        col.setAlignKey(align);
        setValueBinding(col, "sort", sort);

        return col;
    }

    // Get Checkbox component used for select column (multi-select)
    //
    // @param id The component id.
    // @onClickMethod The javascript that get invoked when onClick is captured
    // @param selected Value binding expression for selected property.
    // @param selectedValue Value binding expression for selectedValue property.
    public Checkbox getCheckbox(
        String id,
        String onClickMethod,
        String selected,
        String selectedValue) {
        // Get checkbox.
        Checkbox cb = new Checkbox();
        cb.setId(id);

        if (onClickMethod != null) {
            cb.setOnClick(onClickMethod);
        }

        setValueBinding(cb, "selected", selected);
        setValueBinding(cb, "selectedValue", selectedValue);

        return cb;
    }

    // Get RadioButton component used for select column (single-select)
    //
    // @param id The component id.
    // @onClickMethod The javascript that get invoked when onClick is captured
    // @param selected Value binding expression for selected property.
    // @param selectedValue Value binding expression for selectedValue property.
    public RadioButton getRadioButton(
        String id,
        String onClickMethod,
        String selected,
        String selectedValue) {
        // Get radio button.
        RadioButton rb = new RadioButton();
        rb.setId(id);

        if (onClickMethod != null) {
            rb.setOnClick(onClickMethod);
        }

        setValueBinding(rb, "selected", selected);
        setValueBinding(rb, "selectedValue", selectedValue);

        return rb;
    }

    // Get Hyperlink component.
    //
    // @param id The component id.
    // @param text Value binding expression for text.
    // @param action Method binding expression for action.
    // @param parameter Value binding expression for parameter.
    public Hyperlink getHyperlink(
        String id,
        String text,
        String action,
        String parameter) {
        // Get hyperlink.
        Hyperlink hyperlink = new Hyperlink();
        hyperlink.setId(id);
        setValueBinding(hyperlink, "text", text);
        setMethodBinding(hyperlink, "action", action);

        // Create paramerter.
        UIParameter param = new UIParameter();
        param.setId(id + "_param");
        param.setName("param");
        setValueBinding(param, "value", parameter);
        hyperlink.getChildren().add(param);

        return hyperlink;
    }

    // Get StaticText component.
    //
    // @param text Value binding expression for text.
    public StaticText getText(String text) {
        // Get static text.
        StaticText staticText = new StaticText();
        setValueBinding(staticText, "text", text);

        return staticText;
    }

    // Set TableRowGroup children.
    //
    // @param rowGroup The TableRowGroup component.
    // @param cbSort Value binding expression for cb sort.
    // @param cbSelected Value binding expression for cb selected property.
    // @param cbSelectedValue Value binding expression for cb selectedValue
    // property.
    // @param action The Method binding expression for hyperlink action.
    // @param showHeader Flag indicating to display column header text.
    public void setTableRowGroupChildren(
        TableRowGroup rowGroup,
        TableColumn [] columns) {

        if (rowGroup == null || columns == null || columns.length == 0) {
            return;
        }

        for (int i = 0; i < columns.length; i++) {
            rowGroup.getChildren().add(columns[i]);
        }
    }

    // Helper method to set value bindings.
    //
    // @param component The UIComponent to set a value binding for.
    // @param name The name of the value binding.
    // @param value The value of the value binding.
    protected void setValueBinding(UIComponent component, String name,
            String value) {
        if (value == null) {
            return;
        }
        FacesContext context = FacesContext.getCurrentInstance();
        component.setValueBinding(name, context.getApplication().
            createValueBinding(value));
    }

    // Helper method to set method bindings.
    //
    // @param component The UIComponent to set a value binding for.
    // @param name The name of the method binding.
    // @param action The action of the method binding.
    protected void setMethodBinding(UIComponent component, String name,
            String action) {
        if (action == null) {
            return;
        }
        FacesContext context = FacesContext.getCurrentInstance();
        component.getAttributes().put(name, context.getApplication().
            createMethodBinding(action, new Class[0]));
    }
}
