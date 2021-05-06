fn Node(a, b) {
		return  { data: a, next: b }
}
const head = Node(10, Node(20))
return head.next.data