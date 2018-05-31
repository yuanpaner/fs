#include "yuanHeadAll.h"
#include "yuanLib.h"

using namespace std;

// from in yuanLib_lc.h, almost all

/******************* PRINT ********************/
// for std, except the tree and etc which do not exit in stl


/******************* Array *******************/
template< typename T >
// void printArrary(const T* const array, int cnt) // array name is a constant address for T type 
void printArrary(T* const array, int cnt) // ok
{
	cout << "Start to print the Array...:\n";
	for(int i = 0 ; i < cnt; ++i)
		cout << array[i] << ' ';
	cout << "\n...Array print over\n";
}

/******************* vector *******************/
//version2.0, for vector<vector<>>, c++ 11, matrix will have a good format
template < typename T >
ostream & operator<<(ostream & out, const vector<T> &v)
{
	for(auto & item : v)
		out << setw(20) << item ; //using "\t"  "\n" unable to align
	out << '\n'; 
	return out;
}

template < typename T >
void printVector(const vector<T> & vec) 
{
	for(auto & item : vec)
		cout << setw(8) << item;		
	cout << '\n';
}

/******************* tree ********************/
// template < typename T >
void printTree(TreeNode* & root) //unable to use const
{
	if(!root) 
	{
		cout << "empty tree\n";
		return;
	}

	cout << "\nprint out tree... \n";
	queue<TreeNode*> tQ;
	// vector<vector<string> > res;
	tQ.push(root);
	while(!tQ.empty())
	{
		int sz = tQ.size();
		vector<string> thisLevel;
		for (int i = 0; i < sz; ++i)
		{
			TreeNode* tmp = tQ.front();
			if(!tmp)
			{
				thisLevel.push_back("null");
				tQ.push(NULL);tQ.push(NULL);
			}
			else
			{
				thisLevel.push_back(to_string(tmp->val));
				tQ.push(tmp->left);
				tQ.push(tmp->right);
			}	
			tQ.pop();		
		}
		// res.push_back(thisLevel);

		int i = 0;
		for (; i < thisLevel.size() && thisLevel[i] == "null"; ++i); // error without ";"
		if(i == thisLevel.size()) // this level are all null. to the end
			break;

		printVector(thisLevel);
	}
}

void printTree(TreeNode* && root) //for rval 
{
	if(!root) 
	{
		cout << "empty tree\n";
		return;
	}

	cout << "print out tree \n";
	queue<TreeNode*> tQ;
	// vector<vector<string> > res;
	tQ.push(root);
	while(!tQ.empty())
	{
		int sz = tQ.size();
		vector<string> thisLevel;
		for (int i = 0; i < sz; ++i)
		{
			TreeNode* tmp = tQ.front();
			if(!tmp)
			{
				thisLevel.push_back("null");
				tQ.push(NULL);tQ.push(NULL);
			}
			else
			{
				thisLevel.push_back(to_string(tmp->val));
				tQ.push(tmp->left);
				tQ.push(tmp->right);
			}	
			tQ.pop();		
		}
		// res.push_back(thisLevel);

		int i = 0;
		for (; i < thisLevel.size() && thisLevel[i] == "null"; ++i); // error without ";"
		if(i == thisLevel.size())
			return;

		printVector(thisLevel);
	}

}

//using a root to print the whole tree is not proper, what if i wnat to cout << single tree node
/*
ostream & operator<<(ostream & out, TreeNode* root)
{
	if(!root) 
	{
		out << "mpty tree\n";
		return out;
	}

	out << "print out tree \n";
	queue<TreeNode*> tQ;
	// vector<vector<string> > res;
	tQ.push(root);
	while(!tQ.empty())
	{
		int sz = tQ.size();
		vector<string> thisLevel;
		for (int i = 0; i < sz; ++i)
		{
			TreeNode* tmp = tQ.front();
			if(!tmp)
			{
				thisLevel.push_back("null");
				tQ.push(NULL);tQ.push(NULL);
			}
			else
			{
				thisLevel.push_back(to_string(tmp->val));
				tQ.push(tmp->left);
				tQ.push(tmp->right);
			}	
			tQ.pop();		
		}
		// res.push_back(thisLevel);

		int i = 0;
		for (; i < thisLevel.size() && thisLevel[i] == "null"; ++i); // error without ";"
		if(i == thisLevel.size())
			return out;

		printVector(thisLevel); //not one line for one node
		// out << thisLevel << '\n';
	}
	return out;

}
*/
ostream & operator<<(ostream & out, TreeNode* node)
{
	if(!node) out << "null ";
	else out << node->val << ' ';

	return out;
}
/******************* list ********************/
//for std list
template< typename T > 
void printList(const list<T> &listRef)
{

	if (listRef.empty())
		cout << "list is empty. \n";
	else
	{
		ostream_iterator<T> output(cout, " ");
		copy(listRef.begin(), listRef.end(), output);
		cout << "\t...list print over\n";
	}
	
}

/******************* stack ********************/
template< typename T > 
void printStack(const stack<T> &stkRef)
{
	if (stkRef.empty())
		cout << "stack is empty. \n";
	else
	{
		// ostream_iterator<T> output(cout, " ");
		// copy(stkRef.begin(), stkRef.end(), output); //no begin
		// while(!stkRef.empty()) //const 不能操作！
		// {
		// 	cout << ' ' << stkRef.top();
		// 	stkRef.pop();
		// }
		stack<T> tmpStk(stkRef);
		while(!tmpStk.empty()) 
		{
			cout <<  tmpStk.top() << '\n' ;
			tmpStk.pop();
		}
		cout << "\t...stack print over, from top \n\n";
	}
}

template<typename T>
void printSet(const set<T> &s)
{
	if(s.empty())
		cout << "\nset empty.\n";
	else
	{
		ostream_iterator<T> output(cout, " ");
		copy(s.begin(), s.end(), output);
		cout << '\n';
	}
}
template< typename T > 
void printSet(const unordered_set<T>& us)
{
	if(us.empty())
	{
		cout << "\nunordered_set is empty.\n";
		return;
	}

	ostream_iterator<T> output(cout, " ");
	copy(s.begin(), s.end(), output);
	cout << "\t....unordered_set output done!\n";
	// for(const T& x : us)
	// 	cout << x << ' ';
	// cout << "\tunordered_set output done!\n";
}
template < typename T >
ostream & operator<<(ostream & out, const unordered_set<T> &s)
{
	if(s.empty())
		cout << "\nunordered_set empty.\n";
	else
	{
		for(auto & item : s)
			out << item << ' ';
		out <<endl;
	}
	return out;
}
template < typename T >
ostream & operator<<(ostream & out, const set<T> &s)
{
	if(s.empty())
		cout << "set empty.\n";
	else
	{
		for(auto & item : s)
			out << item << ' ';
		out <<endl;
	}
	return out;
}

template< typename K, typename V > 
void printMap(const unordered_map<K,V>& um)
{
	if(um.empty())
	{
		cout << "\nunordered_map is empty.\n";
		return;
	}
	for(auto x : um)
		cout << x.first << ':' << x.second << ' ';
	cout << "\tunordered_map output done!\n";
}

template<typename K, typename V>
void printMap(const map<K,V> & m)
{
	if(m.empty()) 
	{
		cout << "\nmap is empty\n";
		return;
	}
	for(auto itr : m) //if iterator, itr->first
		cout << itr.first << ":\t" << itr.second << '\n';
	cout << "map output done!\n";
}

template< typename T >
void printPq(const priority_queue<T> & pq)
{
	if(pq.empty())
	{
		cout << "priority-queue is empty.\n";
		return;
	}
	priority_queue<T> tmpQ(pq);
	while(!tmpQ.empty())
	{
		cout << tmpQ.top() << ' ';
		tmpQ.pop();
	}
	cout << "\tmap output done!\n";
}




/******************* PRIME ********************/

bool isPrime(int n ){
    assert(n > 0);
    if(n == 2 || n == 3)
        return true;
    if(n == 1 || (n&1) == 0)
        return false;
    for(int i = 3; i * i <= n; i +=2)
        if(n % i == 0) return false;
    return true;
}
int nextPrime( int n ){
    if((n & 1) == 0) ++n;
    for(; !isPrime(n); n += 2);
    return n;
}
//implements the Sieve of Eratosthenes
vector<bool> sieveOfEratosthenes(int max) 
{
	vector<bool> flags(max + 1, true);
	int cnt = 0;
	//initiate flags[]
	// for (int i = 0; i <= max ; ++i)
	// 	flags[i] = true;

	int prime = 2;
	// Cross off remaining multiples of prime. We can start with (prime*prime), * because if we have a k * prime, where k < prime, this value would have already been crossed off in a prior iteration.
	while(prime * prime <= max) //!!!! point
	{
		/*cross off reamaining mutiples of primes*/
		//from prime * prime to avoid duplicate
		for (int i = prime * prime; i <= max; i += prime)
			flags[i] = false;
		/* find next prime*/
		++prime;
		while(prime < max+1 && !flags[prime])
			++prime;
	}
	return flags;
}


/******************* CALCULATOR ********************/
/******************* gcd ********************/
//Euclid’s Algorithm
//<data structure> page 69
long long GCD(long long m, long long n)
{
	while(n!=0)
	{
		long long r = m % n;
		m = n;
		n = r;
	}
	return m;
}
/************ Power by DC********************/
long long powInt(long long x, int n)
{
    if(!n) return 1;
    if(n == 1 || x == 1) return x;
    if(n&1) return x*(powInt(x*x, n/2)); 
    else return powInt(x*x , n/2);
}


/******************* GENERATOR ********************/
//http://blog.sina.com.cn/s/blog_57de62c00100ltak.html
void getIntVec_noDup(vector<int>& v, unsigned int cnt = 10)
{
	v.resize(cnt);
	for (int i = 0; i < cnt; ++i)
		v[i] = i;
	for (int i = cnt - 1; i > 0 ; --i)
		swap(v[i], v[rand() % i]);
	
	// cout << "get a random non-duplicate vector (0 - " << cnt << " )\n";
	// printVector(v);
}

//generate a binary search tree(int), root is root, cnt is the node cnt
//not balanced
void getBinarySearchTree(TreeNode * & root, int cnt = 10)
{
	srand(time(0));
	root = NULL;
	TreeNode* node = NULL;
	bool dup = false;
	int pos = 0;
	for (int i = 0; i < cnt; ++i)
	{
		int x = rand() % cnt;
		cout << x << ' ';
		// continue ;
		if(root == NULL)
		{
			root = new TreeNode(x, NULL, NULL);
		}
		else 
		{
			node = root;
			TreeNode* pre;
			while(node)
			{
				pre = node;
				if(x < node->val)
					{ node = node->left; pos = -1;}
				else if( x > node->val)
					{ node = node->right; pos = 1; }
				else if(x == node->val)
					{ dup = true; pos = 0; node = NULL;}
			}
			// if(dup == false)
			// 	node = new TreeNode(x);
			// else dup = false;
			if(!pos) continue;
			else if(pos == 1) pre->right = new TreeNode(x, NULL, NULL, pre);
			else pre->left = new TreeNode(x, NULL, NULL, pre);
		}

	}
	cout << '\n';
	// return root;
}

