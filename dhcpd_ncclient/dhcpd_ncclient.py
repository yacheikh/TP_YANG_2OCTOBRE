from ncclient import manager
from binding import dhcpd
from ncclient.operations.rpc import RPCError
from pyangbind.lib.serialise import pybindIETFXMLEncoder
import xml.dom.minidom
import sys

HOST="localhost"
PORT=22
USER="etu"
PASS="-etu-"

# ============================================================================
# ================================ GET-CONFIG ================================
# ============================================================================

# ============================================================================
def get_config(manager, datastore, filter=None):
    reply= manager.get_config(datastore, filter).data_xml
    return xml.dom.minidom.parseString(reply).toprettyxml()

# ============================================================================
def print_config_running(manager):
    print("========== get-config running ========== ")
    print(get_config(manager,"running"))
    

# ============================================================================
def print_config_running_dhcp(manager):
    print("========== get-config running, filter <dhcp/> ========== ")
    print(get_config(manager,"running",("subtree", "<dhcp/")))

# ============================================================================
def print_config_candidate(manager):
    print("========== get-config candidate ========== ")
    print(get_config(manager,"candidate"))

# ============================================================================
def print_config_candidate_dhcp(manager):
    print("========== get-config candidate, filter <dhcp/>  ========== ")
    print(get_config(manager,"candidate", ("subtree", "<dhcp/")))


# ============================================================================
# =============================== EDIT-CONFIG ================================
# ============================================================================

# ============================================================================
def edit_config_address_error(manager):
    print("========== edit-config address error ========== ")
    dhcpd_module = dhcpd()
    try:
        dhcpd_module.dhcp.dns.append("192.368.8.7")
    except ValueError as ve:
        print("Error building dhcpd module:")
        print(ve)

# ============================================================================
def build_three_dns():
    dhcpd_module = dhcpd()
    dhcpd_module.dhcp.dns.append("192.168.8.7")
    dhcpd_module.dhcp.dns.append("192.168.8.8")
    dhcpd_module.dhcp.dns.append("192.168.8.9")
    config_str = ("<config>"
                  + pybindIETFXMLEncoder.serialise(dhcpd_module.dhcp)
                  + "</config>")
    return config_str

# ============================================================================
def edit_config_max_error(manager):
    print("========== edit-config max elements error ========== ")
    conf_dhcp = build_three_dns()
    print(conf_dhcp)
    try:
        manager.edit_config(target='candidate', config=conf_dhcp,
                            test_option='test-then-set')
        print("edit-config --> OK")
    except RPCError as rpcerror:
        print("edit-config --> ERROR")
        print(rpcerror)

# ============================================================================
def build_dhcp_conf_v1():
    pass
    # TODO

# ============================================================================
def build_dhcp_conf_v2():
    pass
    # TODO

# ============================================================================
def edit_config_candidate(manager):
    print("========== edit-config candidate ========== ")
    # TODO

# ============================================================================
def edit_condig_update_gw(manager):
    print("========== edit-config candidate: update default gw ========== ")
    # TODO

# ============================================================================
def edit_config_delete_dhcp(manager):
    print("========== edit-config candidate: delete dhcp ========== ")
    conf_dhcp = """ <config xmlns:nc="urn:ietf:params:xml:ns:netconf:base:1.0">
                    <dhcp xmlns="http://univ-tlse3.fr/ns/dhcpd" nc:operation="delete"/>
                    </config> """
    try:
        manager.edit_config(target='candidate', config=conf_dhcp,
                            test_option='test-then-set')
        print("edit-config --> OK")
    except RPCError as rpcerror:
        print("edit-config --> ERROR")
        print(rpcerror)


# ============================================================================
# ============================ OTHER OPERATIONS ==============================
# ============================================================================

# ============================================================================
def discard_changes(manager):
    print("========== discard changes on candidate ========== ")
    # TODO

# ============================================================================
def validate_candidate(manager):
    print("========== validate candidate ========== ")
    # TODO

# ============================================================================
def commit(manager):
    print("========== commit ========== ")
    # TODO

# ============================================================================
def commit_confirmed(manager):
    print("========== commit with confirmed option (timeout 1min) ========== ")
    # TODO

# ============================================================================
def lock(manager):
    print("========== lock ========== ")
    print("Which datastore: running or candidate? [r or c]")
    input = sys.stdin.readline();
    input = input[:-1] # trim \n
    # default datastore -> running
    datastore = "candidate" if input=="c" else "running"
    try:
        manager.lock(datastore)
        print("lock {} --> OK".format(datastore))
    except RPCError as rpcerror:
        print("lock {} --> ERROR".format(datastore))
        print(rpcerror)

# ============================================================================
def unlock(manager):
    print("========== unlock ========== ")
    print("Which datastore: running or candidate? [r or c]")
    input = sys.stdin.readline();
    input = input[:-1] # trim \n
    # default datastore -> running
    datastore = "candidate" if input=="c" else "running"
    try:
        manager.unlock(datastore)
        print("unlock {} --> OK".format(datastore))
    except RPCError as rpcerror:
        print("unlock {} --> ERROR".format(datastore))
        print(rpcerror)

# ============================================================================
# ================================== MENU ====================================
# ============================================================================

def print_invalid_choice():
    print("Invalid choice!")

def print_menu():
    print("******************** Menu ********************")
    print(" 1) get-config running")
    print(" 2) get-config running <dhcp/>")
    print(" 3) get-config candidate")
    print(" 4) get-config candidate <dhcp/>\n")
    print("10) edit-config error address format")
    print("11) edit-config error max elements")
    print("12) edit-config candidate")
    print("13) edit-config candidate update default router\n")
    print("14) edit-config candidate delete dhcp")
    print("20) discard changes")
    print("21) validate candidate")
    print("22) commit (candidate --> running)")
    print("23) commit confirmed (candidate --> running)")
    print("24) lock")
    print("25) unlock")
    print(" 0) exit\n")
    print("Your choice : ", end="", flush=True)

def switcher(arg, manager):
    dict = {
         1: print_config_running,
         2: print_config_running_dhcp,
         3: print_config_candidate,
         4: print_config_candidate_dhcp,
        10: edit_config_address_error,
        11: edit_config_max_error,
        12: edit_config_candidate,
        13: edit_condig_update_gw,
        14: edit_config_delete_dhcp,
        20: discard_changes,
        21: validate_candidate,
        22: commit,
        23: commit_confirmed,
        24: lock,
        25: unlock
    }
    # Get the function from switcher dictionary
    func = dict.get(arg, "invalid choice")
    # Execute function
    if type(func)==str:
        print("Invalid choice")
    else:
        func(manager)

# ============================================================================
# ================================ MAIN CODE =================================
# ============================================================================

m = manager.connect(host=HOST, port=PORT, username=USER, password=PASS,
                    allow_agent=False, look_for_keys=False, hostkey_verify=False)
print("SSH connection to NETCONF serveur --> OK.")
# print("Server capabilities : ")
# for c in m.server_capabilities:
#     print(c)
try:
    end=False
    while not end:
        print_menu();
        try:
            choice = int(sys.stdin.readline())
            # print("you chose {}".format(choice))
            if (choice==0):
                end=True
            else:
                switcher(choice, m)
        except ValueError:
            print("Enter a valid menu item!")
finally:
    m.close_session()
    print("NETCONF session closed.")
