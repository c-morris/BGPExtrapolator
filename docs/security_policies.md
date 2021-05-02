ROV Reference Policy and Custom Policies {#rov-reference-policy-and-custom-policies .unnumbered}
========================================

Route Origin Validation {#route-origin-validation .unnumbered}
-----------------------

Route Origin Validation (ROV) is a mechanism designed to prevent
unplanned advertisement of routes. It uses Resource Public Key
Infrastructure (RPKI) to make sure that an advertisement originates from
a valid AS. Specifically, ROV queries a database received from an RPKI
cache server that contains prefix-to-AS mappings. An announcement is
valid if its origin AS and prefix are found in the database [@Juniper].
ROV Extrapolator simulates Route Origin Validation.

Creating a Custom Policy Extrapolator {#creating-a-custom-policy-extrapolator .unnumbered}
-------------------------------------

To create a new extrapolator that implements a custom policy, it is
recommended to inherit from every class that the extrapolator uses.
Specifically, a child should be created from each of these classes:

-   Announcement

-   BaseAS

-   BlockedExtrapolator

-   BaseGraph

-   SQLQuerier

### Announcement {#announcement .unnumbered}

If the desired policy uses additional properties or methods for
announcements, they should be added to the children of the Announcement
class. Although it does not use any additional variables,
ROVAnnouncement class provides a good example of inheritance from the
Announcement class.

### AS {#as .unnumbered}

Most likely, the AS class for a new policy will override some methods of
the BaseAS class. For example, ROVAS overrides the
`process_announcement` method to reject announcements per ROV as well as
defines four new functions to handle ROV.

### Extrapolator {#extrapolator .unnumbered}

The new extrapolator will inherit from BlockedExtrapolator and use other
classes mentioned in this section. This class is responsible for
extrapolation, seeding announcements, and several other things. In the
case of ROVExtrapolator, `extrapolate_blocks` is overridden to keep
track of announcements' ROA validity. Furthermore, the ROV version
overrides `give_ann_to_as_path` so that announcements are seeded with
ROV taken into account. Specifically, this function sets
`received_from_asn` of the announcement's origin AS to 64513 when it is
not valid and 64514 when it is valid. These ASNs are private, so there
will be no interference during extrapolation. Later in lib\_bgp\_data,
those numbers enable traceback in SQL.

### Graph {#graph .unnumbered}

ROVASGraph class can be used as a reference for creating a new Graph
class. By default, the `process` function in BaseGraph removes stubs.
That is done because simulation scenarios that do not seed announcements
at edge ASes may want to keep the stubs removed since all stubs will
have the same routing information bases as their providers. In most
cases, the new Graph class should override it because keeping stubs is
preferred for achieving a greater level of detail and for cases when a
stub is an attacker. When data from additional table(s) is needed to
create the graph, `create_graph_from_db` should be overridden.
Additionally, Graph implements the `createNew` function that creates a
new AS. If the new AS class has a constructor with additional
parameters, that function should be overridden as well.

### SQLQuerier {#sqlquerier .unnumbered}

SQLQuerier is the class made for interaction with the database. In cases
when the new extrapolator utilizes tables that differ from the default
ones, new functions should be defined in SQLQuerier's child. For
example, ROVSQLQuerier overrides `select_prefix_ann` and
`select_subnet_ann` functions because the ROV version needs to select an
additional field containing ROA validity. Furthermore, this class
implements the `select_AS_flags` function (used in
`ROVASGraph::create_graph_from_db`) to get ROV adoption for each AS.
