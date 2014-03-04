<?php
/**
 * @file
 * Admin interface and everything related
 */

function fratler_master_admin($form, &$form_state) {
	debug(fratler_master_load_multiple(), 'masters');
	return array();
}
